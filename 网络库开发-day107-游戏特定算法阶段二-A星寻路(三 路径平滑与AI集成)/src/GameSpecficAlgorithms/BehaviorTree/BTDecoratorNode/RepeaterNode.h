#pragma once

#include "BTDecoratorNode.h"

// 重复执行子节点 N 次。
class RepeaterNode : public BTDecoratorNode
{
public:
    explicit RepeaterNode(std::unique_ptr<BTNode> child, int repeat_count = -1)
    :BTDecoratorNode(std::move(child))
    ,repeat_count_(repeat_count)
    ,executed_count_(0)
    { }

    virtual ~RepeaterNode() = default;

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
        if(repeat_count_ < 0)
        {
            child_->Execute(ctx, delta_ms);
            return BTStatus::Running;
        }

        if(executed_count_ >= repeat_count_)
        {
            ResetNode();
            return BTStatus::Success;
        }

        BTStatus result = child_->Execute(ctx, delta_ms);
        if(BTStatus::Running == result)
        {
            return BTStatus::Running;
        }

        executed_count_ += 1;
        if(executed_count_ < repeat_count_)
        {
            return BTStatus::Running;
        }

        ResetNode();
        return BTStatus::Success;
    }

    virtual void ResetNode() override 
    {
        executed_count_ = 0;
        if(child_)
        {
            child_->ResetNode();
        }
    }

    // 获取节点名称（调试用）
    virtual std::string GetName() const override
    {
        return "Repeater";
    }

private:
    int repeat_count_;
    int executed_count_ = 0;
};