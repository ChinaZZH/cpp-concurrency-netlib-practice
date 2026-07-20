#include "ClientPredictionManager.h"
#include "../../TcpConnection.h"
#include "../../Decoder/LengthAndTypePrefixDecoder.h"
#include "../../GameSpecficAlgorithms/GameServerMsgTypeDefine.h"
#include "../../../build/proto_gen/frame_sync.pb.h"

ClientPredictionManager::ClientPredictionManager(uint32_t local_player_id, std::shared_ptr<TcpConnection> tcpConnection)
: local_player_id_(local_player_id)
,tcp_conn_(tcpConnection)
,current_client_frame_(0)
{
    // 初始化本地状态为原点
    local_state_.x = Fixed::Zero();
    local_state_.y = Fixed::Zero();

    local_state_.vx = Fixed::Zero();
    local_state_.vy = Fixed::Zero();

    local_state_.state = 0;
    local_state_.hp = 100;
}



// 1.处理本地输入: 立即预测，并缓存输入
void ClientPredictionManager::OnLocalInput(const ClientInput& input)
{
    // 分配客户端预测帧号（本地自增）
    uint32_t predicted_frame = current_client_frame_ + 1; // 下一帧的输入

    // 存储待确认输入（key = 客户端预测帧号）
    ClientInput copy = input;
    copy.set_frame_index(predicted_frame);
    copy.set_client_seq(predicted_frame);
    {
        std::lock_guard<std::mutex> lk(mutex_);
        pending_inputs_[predicted_frame] = copy; // 存到队列中
    }
    
    std::cout << "predicted_frame: " << predicted_frame << " move_pos=(" << input.move_x() << ", " << input.move_y() << " )" << std::endl;
}


// 2. 每帧 Tick：推进帧号 → 取输入 → 模拟 → 保存快照 → 发数据
void ClientPredictionManager::Tick(uint32_t delta_ms)
{
    // 2.1 推进帧号
    current_client_frame_ += 1;

    // 2.2 从队列中取这一帧的输入（如果没有，就构造空输入）
    bool bQuery = false;

     // 空输入
    ClientInput input;
    input.set_player_id(local_player_id_);
    input.set_frame_index(current_client_frame_);
    input.set_move_x(0);
    input.set_move_y(0);
    input.set_attack(false);
    input.set_skill_id(0);
    input.set_client_seq(current_client_frame_);
    {
        std::lock_guard<std::mutex> lk(mutex_);
        auto itr = pending_inputs_.find(current_client_frame_);
        if(itr != pending_inputs_.end())
        {
            input = itr->second;
            bQuery = true;
        }
    }
    


    // 2.3 执行模拟（用 input 更新 state_）
    Simulate(input, delta_ms);

    //  保存当前帧快照
    snapshots_.Save(current_client_frame_, local_state_);

   

    // 可选：打印日志看效果
    if (bQuery)
    {
         // 2.4 【新增】把这一帧的输入发送给服务端
        SendToSever(input);
       // std::cout << "Frame: " << current_client_frame_ << " pos=(" << local_state_.x.ToFloat() << ", " << local_state_.y.ToFloat() << " )" << std::endl;
    }
}


// 模拟函数：根据输入更新状态（先用简单移动）
void ClientPredictionManager::Simulate(const ClientInput& input, float delta_ms)
{
    float fSpeed = 0.1f; // 每毫秒移动速度(固定速率)
    local_state_.x += Fixed(input.move_x() * fSpeed * delta_ms);
    local_state_.y += Fixed(input.move_y() * fSpeed * delta_ms);

    
}

// 发送数据给服务端（网络层接口）
void ClientPredictionManager::SendToSever(const ClientInput& input)
{
    std::string strContent = std::move(LengthAndTypePrefixDecoder::MakeRequestString(input.SerializeAsString(), GSMT_FrameClientInput));
    tcp_conn_->Send(strContent); // 使用你的 TcpConnection

    /*
    printf("  -> Send to server: frame=%d, move=(%d,%d)\n", 
               input.frame_index(), 
               input.move_x(), 
               input.move_y()
            );
    */
            
}


// 测试临时使用
void ClientPredictionManager::InitTcpConnection(std::shared_ptr<TcpConnection> tcpConnection)
{
    tcp_conn_ = (tcpConnection);
}

void ClientPredictionManager::OnAckReceived(const TestAckPackage& ack)
{
    uint32_t acked_client_frame = ack.acked_client_frame();
    uint32_t server_frame_index = ack.server_frame();

    // 1. 建立映射（为后续回滚准备）
    // client_to_server_map_[acked_client_frame] = server_frame_index;

    /*
    既然暂时没有服务端权威位置，我们用一个“懒触发”策略来验证逻辑：

    在 OnAckReceived（第二步加的）中，如果发现 acked_frame 比 current_frame_ 小很多（比如差距 > 5 帧），
    
    就主动触发一次回滚（模拟断线重连或强制矫正）。
    */

    // 【新增】如果确认的帧远落后于当前帧，触发回滚（模拟矫正）
    if(current_client_frame_ - acked_client_frame > 5)
    {
        printf("[Trigger] Ack lag detected! Rolling back to frame %d\n", acked_client_frame);
        Rollback(acked_client_frame);
        return;
    }

    // 2. 清理 pending_inputs_ 中已确认的帧
    std::lock_guard<std::mutex> lk(mutex_);
    for(auto itr = pending_inputs_.begin(); itr != pending_inputs_.end(); )
    {
        if((itr->first) <= acked_client_frame)
        {
            itr = pending_inputs_.erase(itr);   // 小于等于acked_frame 说明已经被消费
        }
        else{
            ++itr;
        }
    }

    /*
    printf("[Client] Ack received: client_frame=%d, server_frame=%d, pending left=%zu\n",
           acked_client_frame, server_frame_index, pending_inputs_.size());
    */
}


void ClientPredictionManager::Rollback(uint32_t target_frame)
{
     std::lock_guard<std::mutex> lk(mutex_);
    // 1. 从快照中恢复状态
    PlayerSnapshot snapshot;
    if(!snapshots_.Restore(target_frame, snapshot))
    {
        printf("[Rollback] ERROR: frame %d not found in snapshots!\n", target_frame);
        return;
    }

    // 2. 重置当前状态到目标帧
    local_state_ = snapshot;
    printf("[Rollback] State reset to frame %d: pos=(%.2f, %.2f)\n",
           target_frame, local_state_.x.ToFloat(), local_state_.y.ToFloat());


    // 3. 清理 pending_inputs_ 中 <= target_frame 的输入（它们已被确认）
    auto itr = pending_inputs_.begin(); 
    while(itr != pending_inputs_.end() && (itr->first) <= target_frame)
    {
        itr = pending_inputs_.erase(itr);   // 小于等于acked_frame 说明已经被消费
    }

    
    // 4. 重放所有 > target_frame 的输入（重新模拟）
    uint32_t replayed_frames = 0;
    for(; itr != pending_inputs_.end(); ++itr)
    {
        auto frame = itr->first;

        // 重新执行模拟（注意：delta_ms 固定为 20）
        Simulate(itr->second, 20.0f);

        //  保存当前帧快照
        snapshots_.Save(frame, local_state_);

        replayed_frames++;
    }

    if (replayed_frames > 0) {
        printf("[Rollback] Replayed %u frames. New pos=(%.2f, %.2f)\n", 
               replayed_frames, local_state_.x.ToFloat(), local_state_.y.ToFloat());
    }
}