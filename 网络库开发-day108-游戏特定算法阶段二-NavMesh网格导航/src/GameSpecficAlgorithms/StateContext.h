#pragma once

#include "../Common/FixedPoint.h"
#include "../Common/FixedPonitMaxFunc.h"

// 状态上下文：包含 AI 实体的必要数据和引用
struct StateContext
{
    uint32_t entity_id;         // 实体 ID
    Fixed x, y;                 // 当前位置
    Fixed hp;                   // 血量
    
    uint32_t target_id;         // 目标实体 ID（0 表示无目标）
    Fixed target_x, target_y;   // 目标位置（用于巡逻点）

    void* user_data = nullptr;

    // 辅助方法：计算到目标的距离平方（定点数）
    Fixed DistanceToTargetSq() const {
        Fixed dx = x - target_x;
        Fixed dy = y - target_y;
        return dx * dx + dy * dy;
    }


    // 辅助方法：计算到任意点的距离平方（定点数）
    Fixed DistanceToPointSq(Fixed px, Fixed py) const {
        Fixed dx = x - px;
        Fixed dy = y - py;
        return dx * dx + dy * dy;
    }
};