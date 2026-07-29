#pragma once

#include "BTCompositeNode.h"

// 选择节点：依次执行子节点，任意一个成功即成功
class SelectorNode : public BTCompositeNode
{
public:
    virtual ~SelectorNode() = default;

    // 核心执行接口
    // - ctx: 状态上下文（与 FSM 共用 StateContext）
    // - delta_ms: 时间步长（毫秒）
    virtual BTStatus Execute(StateContext& ctx, float delta_ms)
    {
        // 如果所有子节点都已成功执行，返回 Success
        if(current_child_index_ >= children_.size())
        {
            current_child_index_ = 0;               // 重置（便于下次执行）
            return BTStatus::Failure;
        }

        // 执行当前子节点
        BTStatus result = children_[current_child_index_]->Execute(ctx, delta_ms);
        if(BTStatus::Success == result)
        {
            // 子节点成功，整个 Selector 成功，重置索引
            current_child_index_ = 0;
            return BTStatus::Success;
        }
        else if(BTStatus::Failure == result)
        {
            // 当前子节点失败，尝试下一个
            // 下一帧继续执行下一个子节点，返回 Running（表示正在尝试）
            current_child_index_ += 1;               // 重置（便于下次执行）
            return BTStatus::Running;
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
        return "Selector";
    }
};