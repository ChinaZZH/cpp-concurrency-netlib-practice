#pragma once

#include "BTCompositeNode.h"

// 顺序节点：依次执行所有子节点，全部成功才算成功
class SequenceNode : public BTCompositeNode
{
public:
    virtual ~SequenceNode() = default;

    // 核心执行接口
    // - ctx: 状态上下文（与 FSM 共用 StateContext）
    // - delta_ms: 时间步长（毫秒）
    virtual BTStatus Execute(StateContext& ctx, float delta_ms) override
    {
        // 如果所有子节点都已成功执行，返回 Success
        if(current_child_index_ >= children_.size())
        {
            current_child_index_ = 0;               // 重置（便于下次执行）
            return BTStatus::Success;
        }

        // 执行当前子节点
        BTStatus result = children_[current_child_index_]->Execute(ctx, delta_ms);
        if(BTStatus::Success == result)
        {
            current_child_index_ += 1;
            return BTStatus::Running;
        }
        else if(BTStatus::Failure == result)
        {
            // 子节点失败，整个 Sequence 失败，重置索引
            current_child_index_ = 0;               // 重置（便于下次执行）
            return BTStatus::Failure;
        }
        else
        {
            // 子节点还在运行中，保持当前索引，返回 Running
            return BTStatus::Running;
        }
    }

    // 获取节点名称（调试用）
    virtual std::string GetName() const override
    {
        return "Sequence";
    }
};