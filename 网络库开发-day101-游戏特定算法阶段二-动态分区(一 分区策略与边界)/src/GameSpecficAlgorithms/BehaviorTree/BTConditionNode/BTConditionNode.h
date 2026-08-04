#pragma once

#include "../BTNode.h"
#include <vector>
#include <memory>

// 条件节点：纯判断，不产生副作用
class BTConditionNode : public BTNode
{
public:
    virtual ~BTConditionNode() = default;

    // 核心执行接口
    // - ctx: 状态上下文（与 FSM 共用 StateContext）
    // - delta_ms: 时间步长（毫秒）
    virtual BTStatus Execute(StateContext& ctx, float delta_ms) override = 0;

};