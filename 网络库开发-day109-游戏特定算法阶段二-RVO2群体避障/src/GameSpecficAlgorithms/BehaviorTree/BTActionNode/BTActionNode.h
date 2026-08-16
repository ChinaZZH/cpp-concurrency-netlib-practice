#pragma once

#include "../BTNode.h"
#include <vector>
#include <memory>

// 动作节点：执行具体行为，可能跨帧
class BTActionNode : public BTNode
{
public:
    virtual ~BTActionNode() = default;

    // 核心执行接口
    // - ctx: 状态上下文（与 FSM 共用 StateContext）
    // - delta_ms: 时间步长（毫秒）
    virtual BTStatus Execute(StateContext& ctx, float delta_ms) override = 0;

    virtual void ResetNode() override {}

};