#pragma once

#include "BTDecoratorNode.h"

// 不断执行子节点直到返回 Success。
class UntilSuccessNode : public BTDecoratorNode
{
public:
    explicit UntilSuccessNode(std::unique_ptr<BTNode> child)
    :BTDecoratorNode(std::move(child))
    { }

    virtual ~UntilSuccessNode() = default;

    // 核心执行接口
    // - ctx: 状态上下文（与 FSM 共用 StateContext）
    // - delta_ms: 时间步长（毫秒）
    virtual BTStatus Execute(StateContext& ctx, float delta_ms) override
    {
        if(!child_)
        {
            return BTStatus::Failure;
        }

        BTStatus result = child_->Execute(ctx, delta_ms);
        if(BTStatus::Success == result)
        {
            ResetNode();
            return BTStatus::Success;
        }

        return BTStatus::Running;
    }

    // 获取节点名称（调试用）
    virtual std::string GetName() const override
    {
        return "UntilSuccess";
    }

};