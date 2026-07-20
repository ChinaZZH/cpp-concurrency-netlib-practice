// ServerPlayerManager.h
#pragma once
#include <unordered_map>
#include <cstdint>
#include "../../Common/FixedPoint.h"
#include "../../Common/FixedPonitMaxFunc.h"
#include "../../../build/proto_gen/frame_sync.pb.h"

// 玩家权威状态（服务端持有）
struct ServerPlayerState{
    Fixed x, y;             // 位置
    Fixed vx, vy;           // 速度
    int32_t state = 0;      // 0=待机, 1=跑动, 2=跳跃, 3=攻击...
    uint32_t hp = 100;      // 血量（如果需要回滚伤害）
};


class ServerPlayerManager
{
public:
    ServerPlayerManager();

    // 1. 注册玩家（当玩家加入游戏时调用）
    void AddPlayer(uint32_t player_id);

    // 2. 移除玩家（玩家退出时调用）
    void RemovePlayer(uint32_t player_id);
    
    // 3. 获取某个玩家的权威状态（用于下发给客户端）
    bool GetPlayerState(uint32_t player_id, ServerPlayerState& out) const;

    // 4. 核心：每帧 Tick，推进所有玩家的状态
    //    参数：当前服务端帧号，所有玩家在这一帧的输入列表，时间步长(ms)
    void Tick(uint32_t server_frame, const std::unordered_map<uint32_t, ClientInput>& input_per_player, uint32_t delta_ms);


private:
    // 内部模拟函数（与客户端完全一致） 
    ServerPlayerState Simulate(const ServerPlayerState& current, const ClientInput& input, uint32_t delta_ms);

private:
    // 存储所有玩家的权威状态
    std::unordered_map<uint32_t, ServerPlayerState>  players_;

};