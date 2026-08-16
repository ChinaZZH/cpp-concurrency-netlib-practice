#pragma once

#include "BehaviorTree.h"
#include "../../Common/FixedPoint.h"
#include "../../Common/FixedPonitMaxFunc.h"
#include "../StateContext.h"
#include <string>

// 节点基类
class BTNode
{
public:
    virtual ~BTNode() = default;

    // 核心执行接口
    // - ctx: 状态上下文（与 FSM 共用 StateContext）
    // - delta_ms: 时间步长（毫秒）
    virtual BTStatus Execute(StateContext& ctx, float delta_ms) = 0;

    // 重置节点状态（用于树的重置）
    virtual void ResetNode() {}

    // 获取节点名称（调试用）
    virtual std::string GetName() const = 0;
};