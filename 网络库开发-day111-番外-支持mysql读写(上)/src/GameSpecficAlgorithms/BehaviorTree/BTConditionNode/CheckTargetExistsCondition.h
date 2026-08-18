#pragma once

#include "BTConditionNode.h"


// 检查目标是否存在
class CheckTargetExistsCondition : public BTConditionNode
{
public:
    virtual ~CheckTargetExistsCondition() = default;

    // 核心执行接口
    // - ctx: 状态上下文（与 FSM 共用 StateContext）
    // - delta_ms: 时间步长（毫秒）
    virtual BTStatus Execute(StateContext& ctx, float delta_ms) override 
    {
        return (0 != ctx.target_id) ? BTStatus::Success : BTStatus::Failure;
    }

    virtual std::string GetName() const override {
        return "CheckTargetExists";
    }
};