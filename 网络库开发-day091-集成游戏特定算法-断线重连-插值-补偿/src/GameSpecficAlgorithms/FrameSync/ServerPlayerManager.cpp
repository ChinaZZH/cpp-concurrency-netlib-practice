#include "ServerPlayerManager.h"

ServerPlayerManager::ServerPlayerManager(SendCorrectionCallback cb)
:send_correction_cb_(cb)
{
    
}

 
// 1. 注册玩家（当玩家加入游戏时调用）
void ServerPlayerManager::AddPlayer(uint32_t player_id)
{
    auto itr = players_.find(player_id);
    if(itr != players_.end())
    {
        return;
    }


    // 初始化本地状态为原点
    ServerPlayerState initState;
    initState.x = Fixed::Zero();
    initState.y = Fixed::Zero();

    initState.vx = Fixed::Zero();
    initState.vy = Fixed::Zero();

    initState.state = 0;
    initState.hp = 100;
    players_[player_id] =  initState;
    std::cout << "[Server] Player " << player_id << " registered." << std::endl;
}

// 2. 移除玩家（玩家退出时调用）
void ServerPlayerManager::RemovePlayer(uint32_t player_id)
{
    players_.erase(player_id);
    pending_inputs_.erase(player_id);
}
    
// 3. 获取某个玩家的权威状态（用于下发给客户端）
bool ServerPlayerManager::GetPlayerState(uint32_t player_id, ServerPlayerState& out) const
{
    auto itr = players_.find(player_id);
    if(itr == players_.end())
    {
        return false;
    }

    out = (itr->second);
    return true;
}


// 内部模拟函数（与客户端完全一致） 
ServerPlayerState ServerPlayerManager::Simulate(const ServerPlayerState& current, const ClientInput& input, uint32_t delta_ms)
{
    ServerPlayerState newPlayerState = current;

    float fSpeed = 0.1f; // 每毫秒移动速度(固定速率)
    newPlayerState.x += Fixed(input.move_x() * fSpeed * delta_ms);
    newPlayerState.y += Fixed(input.move_y() * fSpeed * delta_ms);
    return newPlayerState;
}


 // 由 TcpServer 的 OnMessage 调用，将客户端上报的输入暂存起来
void ServerPlayerManager::SumbitInput(uint32_t player_id, const ClientInput& input)
{
    PendingInput pendingData;
    pendingData.input = input;
    pendingData.client_frame = input.frame_index();
    pending_inputs_[player_id] = pendingData;
}


// 4. 核心：每帧 Tick，推进所有玩家的状态
//    参数：当前服务端帧号，所有玩家在这一帧的输入列表，时间步长(ms)
void ServerPlayerManager::Tick(uint32_t delta_ms)
{
    std::unordered_map<uint32_t, PendingInput>  inputs; 
    inputs.swap(pending_inputs_);
    

    // 遍历所有玩家
    for(auto& [player_id, playerState] : players_)
    {
        // ===== 1. 提取输入（含预测坐标） =====
        ClientInput input;
        uint32_t clientFrame = 0;
        bool hasPrediction = false;
        Fixed predictedX, predictedY;
        auto itr = inputs.find(player_id);
        if(itr != inputs.end())
        {
            hasPrediction = true;
            input = (itr->second).input;
            clientFrame = (itr->second).client_frame;
            predictedX = Fixed::FromRaw(input.predicted_x());
            predictedY = Fixed::FromRaw(input.predicted_y());
        }
        else
        {
             // 空输入：所有操作归零
            input.set_player_id(player_id);
            input.set_frame_index(0);
            input.set_move_x(0);
            input.set_move_y(0);
            input.set_attack(false);
            input.set_skill_id(0);
        }

        // ===== 2. 执行权威模拟 =====
        ServerPlayerState oldPlayerState = playerState;
        playerState = Simulate(playerState, input, delta_ms);

        if(nullptr == send_correction_cb_)
        {
            std::cout << "[Server] send_correction_cb_ is nullptr.\n";
            continue;
        }

        // ===== 3. 校验对比（如果客户端有上报预测位置） =====
        if(hasPrediction)
        {
            Fixed dx = playerState.x - predictedX;
            Fixed dy = playerState.y - predictedY;
            if(dx < Fixed::Zero())
            {
                dx = -dx;
            }

            if(dy < Fixed::Zero())
            {
                dy = -dy;
            }


            // 【正确写法】设定阈值为 0.02 单位，MOBA/FPS 通用
            Fixed threshold = Fixed(0.02f);
            if(dx > threshold || dy > threshold)
            {
                // 【关键】只记录日志，暂不下发矫正
                /*
                printf("[Server] Player %u MISMATCH! Server=(%.4f, %.4f), Client=(%.4f, %.4f), Diff=(%.4f, %.4f)\n",
                       player_id,
                       playerState.x.ToDouble(), playerState.y.ToDouble(),
                       predictedX.ToDouble(), predictedY.ToDouble(),
                       dx.ToDouble(), dy.ToDouble());
                */

               std::cout << "[Server] MISMATCH! Sending correction.\n";

               ServerCorrection correction;
               correction.set_player_id(player_id);
               correction.set_client_frame(clientFrame);
               correction.set_authoritative_x(playerState.x.Raw());
               correction.set_authoritative_y(playerState.y.Raw());

               std::string strData;
               correction.SerializeToString(&strData);
               send_correction_cb_(player_id, strData);

            }
            else{
                std::cout << "[Server] OK! Sending correction.\n";
            }
        }
    }
}