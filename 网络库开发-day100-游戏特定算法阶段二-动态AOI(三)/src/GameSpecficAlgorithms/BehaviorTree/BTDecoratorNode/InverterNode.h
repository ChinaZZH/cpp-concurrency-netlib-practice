#pragma once

#include "BTDecoratorNode.h"

// 反转子节点的 Success/Failure。
class InverterNode : public BTDecoratorNode
{
public:
    explicit InverterNode(std::unique_ptr<BTNode> child)
    :BTDecoratorNode(std::move(child))
    { }

    virtual ~InverterNode() = default;

    // 核心执行接口
    // - ctx: 状态上下文（与 FSM 共用 StateContext）
    // - delta_ms: 时间步长（毫秒）
    virtual BTStatus Execute(StateContext& ctx, float delta_ms) override
    {
        if(!child_)
        {
            return BTStatus::Failure;
        }

        // 执行当前子节点
        BTStatus result = child_->Execute(ctx, delta_ms);
        if(BTStatus::Success == result)
        {
            return BTStatus::Failure;
        }

        if(BTStatus::Failure == result)
        {
            return BTStatus::Success;
        }
        
        return BTStatus::Running;
    }

    // 获取节点名称（调试用）
    virtual std::string GetName() const override
    {
        return "Inverter";
    }
};