#include "FrameScheduler.h"
#include <chrono>
#include "../../../build/proto_gen/frame_sync.pb.h"


// ============================================================
// 1. 物理逻辑 Tick（20ms 触发）
// ============================================================
 void FrameScheduler::OnLogicTick()
 {
    // 1.2 从 InputBuffer 获取本帧所有玩家的输入（只读，不删除）
    auto inputs = buffer_->FetchFrame(this->GetServerFrameIndex());


    // ============================================================
    // 第三步：驱动物理计算
    // ============================================================
    server_player_mgr_->Tick(inputs, 20);
 }


// ============================================================
// 2. 网络广播 Tick（50ms 触发）
// ============================================================
void FrameScheduler::OnBroadcastTick()
{
    // 1. 推进帧号 + 清理过期缓存
    uint32_t server_frame_index = AddServerFrameIndex();
    buffer_->Gc(server_frame_index);

    // ========== 2.1 发送矫正包（单播） ==========
    const auto& corrections = server_player_mgr_->GetPendingCorrections();
    for(const auto& [player_id, entry] : corrections)
    {
        // 通过回调发送给对应客户端（具体网络发送由上层实现）
        correction_cb_(player_id, entry);
        
        printf("[Broadcast] Correction sent to player %u, client_frame=%u, pos=(%.2f, %.2f)\n",
               player_id, entry.client_frame,
               entry.state.x.ToDouble(), entry.state.y.ToDouble());
    }

    // 清空矫正缓存（防止下一轮重复发送）
    server_player_mgr_->ClearCorrections();

    // ========== 2.2 构造并广播帧包（FramePackage） ==========
    // 注意：这里使用当前的 server_frame_（只读），不递增
    auto inputs_per_player = buffer_->FetchFrame(server_frame_index_);

    // 3. 构造帧包（即使空包也要推进帧号）
    uint64_t currentTime = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());

    FramePackage framePkg;
    framePkg.set_frame_index(server_frame_index_);
    framePkg.set_timestamp_ms(currentTime);
    for(auto& input : inputs_per_player) 
    {
        auto* newInput = framePkg.add_inputs();
        *newInput = (input.second); // protobuf 支持直接赋值
        //std::cout << "FrameScheduler::OnTick x:=" << newInput->move_x() << " y:=" << newInput->move_y() <<std::endl;
    }
    
    // 4. 序列化并广播
    std::string serialized_pkg;
    if(framePkg.SerializeToString(&serialized_pkg))
    {
        if(broadcastCallback_)
        {       
            broadcastCallback_(serialized_pkg);
        }       
    }


   std::cout << "[Broadcast] Frame " << server_frame_index_ << " broadcasted. ";
   std::cout <<  inputs_per_player.size() << " inputs, " << corrections.size() << " corrections.\n";
}



uint32_t FrameScheduler::AddServerFrameIndex()
 {
    return server_frame_index_.fetch_add(1, std::memory_order_relaxed);
 }
