#include "ServerPlayerManager.h"

ServerPlayerManager::ServerPlayerManager()
{
    // 无需初始化
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

// 4. 核心：每帧 Tick，推进所有玩家的状态
//    参数：当前服务端帧号，所有玩家在这一帧的输入列表，时间步长(ms)
void ServerPlayerManager::Tick(uint32_t server_frame, const std::unordered_map<uint32_t, ClientInput>& input_per_player
    , uint32_t delta_ms)
{
    for(auto& [player_id, playerState] : players_)
    {
        ClientInput input;
        auto itr = input_per_player.find(player_id);
        if(itr != input_per_player.end())
        {
            input = (itr->second);
        }
        else
        {
             // 空输入：所有操作归零
            input.set_player_id(player_id);
            input.set_frame_index(server_frame);
            input.set_move_x(0);
            input.set_move_y(0);
            input.set_attack(false);
            input.set_skill_id(0);
        }

        
        playerState = Simulate(playerState, input, delta_ms);
    }
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