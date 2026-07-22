#pragma once

#include <cstdint>
#include "../../Common/FixedPoint.h"
#include "../../Common/FixedPonitMaxFunc.h"

// 远程玩家的一个状态快照（用于插值）
struct RemoteStateSnapshot 
{
    Fixed x, y;                 // 位置（其他状态如速度、朝向可后续扩展）
    uint64_t timeStamp_ms;      // 该状态对应的时间戳（毫秒）
    bool valid = false;         // 是否有效
};


// 为单个远程玩家维护双缓冲状态，支持插值
class RemotePlayerSmoother
{
public:
    RemotePlayerSmoother() = default;

    // 1. 推送新状态（由网络层收到服务器广播时调用）
    void PushState(const RemoteStateSnapshot& newState);

    // 2. 获取用于渲染的插值状态（由渲染循环调用）
    RemoteStateSnapshot GetRenderState(uint64_t now_ms) const;

    // 3. 检查是否有足够数据进行插值
    bool IsReady() const { return prev_.valid && next_.valid; }

private:
    // 线性插值辅助函数
    Fixed Lerp(Fixed a, Fixed b, float t) const;

private:
    RemoteStateSnapshot prev_;  // 较早的状态
    RemoteStateSnapshot next_;  // 较晚的状态
};