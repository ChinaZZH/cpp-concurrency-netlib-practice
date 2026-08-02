#pragma once

#include "BTDecoratorNode.h"

// 限制子节点执行时间，超时则返回 Failure
class TimeoutNode : public BTDecoratorNode
{
public:
    explicit TimeoutNode(std::unique_ptr<BTNode> child, float timeout_ms)
    :BTDecoratorNode(std::move(child))
    , elapsed_ms_(0.00f)
    ,timeout_ms_(timeout_ms)
    { }

    virtual ~TimeoutNode() = default;

    // 核心执行接口
    // - ctx: 状态上下文（与 FSM 共用 StateContext）
    // - delta_ms: 时间步长（毫秒）
    virtual BTStatus Execute(StateContext& ctx, float delta_ms) override
    {
        if(!child_)
        {
            return BTStatus::Failure;
        }

        // 无限重复
        BTStatus result = child_->Execute(ctx, delta_ms);
        if(BTStatus::Running != result)
        {
            ResetNode();
            return result;
        }

        elapsed_ms_ += delta_ms;
        if(elapsed_ms_ >= timeout_ms_)
        {
            ResetNode();
            return BTStatus::Failure;
        }

        return BTStatus::Running;
    }

    virtual void ResetNode() override 
    {
        elapsed_ms_ = 0.00f;
        if(child_)
        {
            child_->ResetNode();
        }
    }

    // 获取节点名称（调试用）
    virtual std::string GetName() const override
    {
        return "Timeout";
    }

private:
    float elapsed_ms_;
    float timeout_ms_;
};