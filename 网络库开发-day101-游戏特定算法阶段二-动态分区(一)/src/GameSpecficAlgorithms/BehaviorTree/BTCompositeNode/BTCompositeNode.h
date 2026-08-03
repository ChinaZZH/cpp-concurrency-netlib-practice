#pragma once

#include "../BTNode.h"
#include <vector>
#include <memory>

// 组合节点基类：管理子节点列表
class BTCompositeNode : public BTNode
{
public:
    virtual ~BTCompositeNode() = default;

    // 添加子节点
    void AddChild(std::unique_ptr<BTNode> child)
    {
        children_.push_back(std::move(child));
    }

    // 重置节点状态（用于树的重置）
    virtual void ResetNode() override
    {
        current_child_index_ = 0;
        for(auto& node : children_)
        {
            node->ResetNode();
        }
    }

protected:
    std::vector<std::unique_ptr<BTNode>> children_;
    size_t current_child_index_ = 0;                    // 当前正在执行的子节点索引
};