#pragma once

#include "BTConditionNode.h"

// 检查到目标距离是否小于阈值
class CheckDistanceCondition : public BTConditionNode
{
public:
    explicit CheckDistanceCondition(float threshold)
    :threshold_(Fixed(threshold))
    { }

    virtual ~CheckDistanceCondition() = default;

    // 核心执行接口
    // - ctx: 状态上下文（与 FSM 共用 StateContext）
    // - delta_ms: 时间步长（毫秒）
    virtual BTStatus Execute(StateContext& ctx, float delta_ms) override 
    {
        if(0 == ctx.target_id)
        {
            return BTStatus::Failure;
        }

        Fixed sqrtThreshold = threshold_ * threshold_;
        Fixed distance = ctx.DistanceToTargetSq();
        if(distance >= sqrtThreshold)
        {
            return BTStatus::Failure;
        }

        return BTStatus::Success;
    }

    virtual std::string GetName() const override {
        return "CheckDistance";
    }

private:
    Fixed threshold_;
};