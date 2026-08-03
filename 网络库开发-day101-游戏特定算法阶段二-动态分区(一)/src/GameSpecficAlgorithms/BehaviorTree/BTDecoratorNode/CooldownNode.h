#pragma once

#include "BTDecoratorNode.h"

// 限制节点执行频率（冷却时间）
class CooldownNode : public BTDecoratorNode
{
public:
    explicit CooldownNode(std::unique_ptr<BTNode> child, float cooldown_ms)
    :BTDecoratorNode(std::move(child))
    , remain_cooldown_ms_(0.0f)
    ,cooldown_ms_(cooldown_ms)
    { }

    virtual ~CooldownNode() = default;

    // 核心执行接口
    // - ctx: 状态上下文（与 FSM 共用 StateContext）
    // - delta_ms: 时间步长（毫秒）
    virtual BTStatus Execute(StateContext& ctx, float delta_ms) override
    {
        if(!child_)
        {
            return BTStatus::Failure;
        }

        if(remain_cooldown_ms_ > delta_ms)
        {
            remain_cooldown_ms_ -= delta_ms;
            return BTStatus::Running;
        }


        remain_cooldown_ms_ = 0.00f;
        BTStatus result = child_->Execute(ctx, delta_ms);
        if(BTStatus::Running != result)
        {
            remain_cooldown_ms_ = cooldown_ms_;
            return result;
        }
        
        return BTStatus::Running;
    }

    virtual void ResetNode() override 
    {
        remain_cooldown_ms_ = 0.0f;
        if(child_)
        {
            child_->ResetNode();
        }
    }

    // 获取节点名称（调试用）
    virtual std::string GetName() const override
    {
        return "Cooldown";
    }

private:
    float remain_cooldown_ms_;
    float cooldown_ms_;
};