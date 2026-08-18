#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include "GridNode.h"

namespace A_Star
{
    class NodeManager
    {
    public:
        NodeManager(uint32_t width, uint32_t height);

        GridNode_Ptr GetNode(uint32_t x, uint32_t y);
        const GridNode_Ptr GetConstNode(uint32_t x, uint32_t y) const;

        void ResetAll();
        void ResetNode(uint32_t x, uint32_t y);

        std::vector<GridNode_Ptr> GetAllNodes() { return nodes_list_; }
        uint32_t GetWidth() const { return width_; }
        uint32_t GetHeight() const { return height_; }

    private:
        uint32_t    width_;
        uint32_t    height_;
        std::vector<GridNode_Ptr> nodes_list_;
    };
}