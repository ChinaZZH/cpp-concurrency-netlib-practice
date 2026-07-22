#pragma once

#include <unordered_map>
#include <cstdint>
#include "../../Common/FixedPoint.h"
#include "../../Common/FixedPonitMaxFunc.h"
#include "RemotePlayerSmoother.h"

// 管理所有远程玩家的平滑器
class RemotePlayerManager 
{
public:
    RemotePlayerManager() = default;

    // 1. 更新远程玩家的状态（由网络层收到广播时调用）
    //    timestamp_ms: 服务端生成该状态的时间（毫秒）
    void UpdateRemoteState(uint32_t player_id, Fixed x, Fixed y, uint64_t timestamp_ms);

    // 2. 获取远程玩家当前应渲染的位置（由渲染循环调用）
    //    返回值：true 表示有有效插值结果，out_x/out_y 被填充
    //            false 表示无有效数据，调用方应使用兜底值或跳过渲染
    bool GetRenderPosition(uint32_t player_id, uint64_t now_ms, Fixed& out_x, Fixed& out_y) const;

    // 3. 移除玩家（玩家退出时调用）
    void RemovePlayer(uint32_t player_id);

    // 4. 清空所有（断线重连时调用）
    void Clear();

private:
    std::unordered_map<uint32_t, RemotePlayerSmoother> smoothers_;
};
