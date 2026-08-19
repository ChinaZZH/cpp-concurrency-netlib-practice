#include "NodeManager.h"
#include <iostream>

namespace A_Star
{
    NodeManager::NodeManager(uint32_t width, uint32_t height)
    :width_(width)
    ,height_(height)
    ,nodes_list_(width*height)
    {
       for(int y = 0; y < height_; ++y)
       {
            int base_index = y * width_;
            for(int x = 0; x < width_; ++x)
            {
                int index = base_index + x;
                nodes_list_[index] = std::make_shared<GridNode>();
                nodes_list_[index]->x = x;
                nodes_list_[index]->y = y;
                nodes_list_[index]->Reset();
            }
       }
    }

    GridNode_Ptr NodeManager::GetNode(uint32_t x, uint32_t y)
    {
        const GridNode_Ptr const_node_ptr = this->GetConstNode(x, y);
        GridNode_Ptr common_node_ptr = const_node_ptr;
        return common_node_ptr;
    }

    const GridNode_Ptr NodeManager::GetConstNode(uint32_t x, uint32_t y) const
    {
        if(x >= width_ || y >= height_)
        {
            return nullptr;
        }

        // x 和 y转化为index
        int index = y * width_ + x;
        if(index < 0 || index >= nodes_list_.size())
        {
            return nullptr;
        }

        return nodes_list_[index];
    }

    void NodeManager::ResetAll()
    {
        for(auto& node_ptr : nodes_list_)
        {
            if(node_ptr)
            {
                node_ptr->Reset();
            }
        }
    }
    
    void NodeManager::ResetNode(uint32_t x, uint32_t y)
    {
        GridNode_Ptr node_ptr = this->GetNode(x, y);
        if(!node_ptr)
        {
            return;
        }

        node_ptr->Reset();
    }
}