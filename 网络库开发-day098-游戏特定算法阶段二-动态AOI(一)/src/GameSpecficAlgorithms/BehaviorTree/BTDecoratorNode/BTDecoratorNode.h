#pragma once

#include "../BTNode.h"
#include <vector>
#include <memory>

// 装饰节点基类：管理单个子节点

// 装饰节点（Decorator Node）是只有一个子节点的节点，它在子节点执行前后或执行过程中插入额外的逻辑，而不改变子节点的核心功能。

// 装饰节点是行为树中的“修饰器”，它们不改变子节点的核心行为，而是在子节点执行前后或执行过程中附加额外的控制逻辑，
// 例如：限制执行次数、反转结果、添加冷却时间、设置超时等。

class BTDecoratorNode : public BTNode
{
public:
    explicit BTDecoratorNode(std::unique_ptr<BTNode> child)
    :child_(std::move(child))
    { }

    virtual ~BTDecoratorNode() = default;

    // 重置节点状态（用于树的重置）
    virtual void ResetNode() override
    {
        if(child_)
        {
            child_->ResetNode();
        }
    }

protected:
    std::unique_ptr<BTNode> child_;
};