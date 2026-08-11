#pragma once

#include <cstdint>
#include <memory>

namespace A_Star
{
    struct GridNode
    {
        int x = 0;
        int y = 0;

        int src_to_cur = 0;        // 起点到当前节点的实际代价
        int cur_to_target = 0;     // 当前节点到终点的估计代价
        int final_total = 0;       //  g + h

        GridNode* parent_node = nullptr; // 路径回调用

        bool in_open_list = false;
        bool in_closed_list = false;

        void Reset()
        {
            src_to_cur = 0;        // 起点到当前节点的实际代价
            cur_to_target = 0;     // 当前节点到终点的估计代价
            final_total = 0;       //  g + h

            parent_node = nullptr; // 路径回调用
            in_open_list = false;
            in_closed_list = false;
        }

        bool operator>(const GridNode& other) const {
            return final_total > other.final_total;
        }
    };



    using GridNode_Ptr = std::shared_ptr<GridNode>;
}