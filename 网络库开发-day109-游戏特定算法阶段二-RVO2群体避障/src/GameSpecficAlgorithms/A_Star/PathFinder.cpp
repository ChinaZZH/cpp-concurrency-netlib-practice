#include "PathFinder.h"
#include <algorithm>
#include <iostream>
#include <queue>
#include <vector>
#include <cmath>

namespace A_Star
{
    PathFinder::PathFinder(std::shared_ptr<GridMap> map, std::shared_ptr<NodeManager> node_mgr)
    :map_(map)
    ,node_mgr_(node_mgr)
    {

    }


    PathResult PathFinder::FindPath(int32_t start_x, int32_t start_y, int32_t end_x, int32_t end_y, bool allow_diagonal /*= false*/)
    {
        canceled_ = false;
        PathResult result;

        // 参数校验
        if(!map_ || !node_mgr_)
        {
            return result;
        }

        if(false == this->IsWalkable(start_x, start_y) || false == this->IsWalkable(end_x, end_y))
        {
            std::cout << "[AStar] Start or end is not walkable" << std::endl;
            return result;
        }

        // 2. 获取起点和终点节点
        GridNode_Ptr start_node = node_mgr_->GetNode(start_x, start_y);
        GridNode_Ptr end_node = node_mgr_->GetNode(end_x, end_y);
        if(!start_node || !end_node)
        {
            return result;
        }


         // 如果起点就是终点
        if(start_x == end_x && start_y == end_y)
        {
            return ReconstructPath(start_node);
        }

        // 3. 重置所有节点状态
        node_mgr_->ResetAll();

        // 4. 初始化起点
        start_node->src_to_cur = 0.0f;
        start_node->cur_to_target = Heuristic(start_x, start_y, end_x, end_y);
        start_node->final_total = ((start_node->src_to_cur) + (start_node->cur_to_target));
        start_node->parent_node = nullptr;
        start_node->in_open_list = true;

        // 5. 开放列表 最小堆
        std::priority_queue<GridNode_Ptr, std::vector<GridNode_Ptr>, std::greater<GridNode_Ptr>> open_list;
        open_list.push(start_node);

        std::vector<GridNode_Ptr> neighbors;
        uint32_t explored = 0;

        // 6. 主搜索循环
        while(false == open_list.empty())
        {
            // 取消搜索
            if(canceled_)
            {
                break;
            }

            // a. 从开放列表中取出 f 值最小的节点（当前节点）
            GridNode_Ptr top_node_ptr = open_list.top();
            open_list.pop();
            if(top_node_ptr->in_closed_list)
            {
                continue;
            }

            // b. 将当前节点移入关闭列表
            top_node_ptr->in_closed_list = true;
            top_node_ptr->in_open_list = false;
            explored += 1;

            // c. 如果当前节点 == 终点，跳出循环（路径已找到）
            if(top_node_ptr == end_node)
            {
               result = this->ReconstructPath(end_node);
               result.nodes_explored = explored;
               return result;
            }

            // d. 遍历当前节点的邻居（上下左右）
            neighbors.clear();
            this->GetNeighbors(top_node_ptr, neighbors, allow_diagonal);
            for(auto& neighbor_node : neighbors)
            {
                // d.1. - 如果邻居不可行走 或 在关闭列表中，跳过
                if(neighbor_node->in_closed_list || false == this->IsWalkable(neighbor_node->x, neighbor_node->y))
                {
                    continue;
                }

                // d.2. - 计算通过当前节点到达邻居的 g 值
                int deltaX = std::abs(neighbor_node->x - top_node_ptr->x);
                int deltaY = std::abs(neighbor_node->y - top_node_ptr->y);
                
                float move_cost = 1.0f;
                if(allow_diagonal && deltaX >= 1 && deltaY >= 1)
                {
                    move_cost = diagonal_cost_;
                }

                float new_g = (top_node_ptr->src_to_cur) + move_cost;

                // d.3. - 如果邻居不在开放列表中，加入
                if(false == neighbor_node->in_open_list)
                {
                    neighbor_node->src_to_cur = new_g;
                    neighbor_node->cur_to_target = Heuristic(neighbor_node->x, neighbor_node->y, end_x, end_y);
                    neighbor_node->final_total = ((neighbor_node->src_to_cur) + (neighbor_node->cur_to_target));
                    neighbor_node->parent_node = top_node_ptr;
                    neighbor_node->in_open_list = true;
                    open_list.push(neighbor_node);
                }
                else if(new_g < (neighbor_node->src_to_cur))
                {
                    // - 如果邻居已在开放列表中且新的 g 值更小，更新 g 值和父指针
                    neighbor_node->src_to_cur = new_g;
                    neighbor_node->cur_to_target = Heuristic(neighbor_node->x, neighbor_node->y, end_x, end_y);
                    neighbor_node->final_total = ((neighbor_node->src_to_cur) + (neighbor_node->cur_to_target));
                    neighbor_node->parent_node = top_node_ptr;
                }
            }
        }

        // 开放列表为空，未找到路径
        result.found = false;
        result.nodes_explored = explored;
        std::cout << "[AStar] No path found" << std::endl;
        return result;
    }


     // 启发式函数
    float PathFinder::Heuristic(int32_t src_x, int32_t src_y, int32_t target_x, int32_t target_y) const
    {
        int delta_x = std::abs(src_x - target_x);
        int delta_y = std::abs(src_y - target_y);

        switch(heuristic_type_)
        {
        case HeuristicType::Manhattan:
            return (delta_x + delta_y);

        case HeuristicType::Euclidean:
            return std::sqrt(delta_x * delta_x + delta_y * delta_y);
        
        /*
        在允许走斜线的网格上，把它拆解为“走斜线”和“走直线”两部分来计算，
        其实就是在构建一个由直线和对角线组成的“最短路径框架”——在这个框架里。
        两条相等的边x轴和y轴的可以构成等腰直角三角形，也就是正方形的一般，则它的斜边部分就是变成的根号2. std::min(delta_x, delta_y) * 根号2.
        剩下的部分用直线走 长度就是 std::abs(delta_x- delta_y).
        而启发式的目的，正是为了在搜索开始前就给出这个“最优框架”的估计值, 其实就是拆分。
        */

        case HeuristicType::Diagonal:
            return std::min(delta_x, delta_y) * diagonal_cost_ + std::abs(delta_x - delta_y);

        default:
            return (delta_x + delta_y);
        }

        return 0.0f;
    }

    // 邻居生成
    void PathFinder::GetNeighbors(GridNode_Ptr node, std::vector<GridNode_Ptr>& neighbors, bool allow_diagonal) const
    {
        neighbors.clear();
        int32_t src_x = node->x;
        int32_t src_y = node->y;

        std::vector<std::pair<int32_t, int32_t>> vecDeltaPos;
        if(allow_diagonal)
        {
            // 八方向 以 x轴正轴 为起点方向顺时针方向
            vecDeltaPos.reserve(8);
            vecDeltaPos.emplace_back(std::pair(1, 0));
            vecDeltaPos.emplace_back(std::pair(1, -1));

            vecDeltaPos.emplace_back(std::pair(0, -1));
            vecDeltaPos.emplace_back(std::pair(-1, -1));

            vecDeltaPos.emplace_back(std::pair(-1, 0));
            vecDeltaPos.emplace_back(std::pair(-1, 1));

            vecDeltaPos.emplace_back(std::pair(0, 1));
            vecDeltaPos.emplace_back(std::pair(1, 1));
        }
        else{
            // 四方向 以 x轴正轴 为起点方向顺时针方向
            vecDeltaPos.reserve(4);
            vecDeltaPos.emplace_back(std::pair(1, 0));
            vecDeltaPos.emplace_back(std::pair(0, -1));
            vecDeltaPos.emplace_back(std::pair(-1, 0));
            vecDeltaPos.emplace_back(std::pair(0, 1));
        }


        for(const auto& delta : vecDeltaPos)
        {
            int neighbor_x = src_x + delta.first;
            int neighbor_y = src_y + delta.second;
            if(false == this->IsWalkable(neighbor_x, neighbor_y))
            {
                continue;
            }

            GridNode_Ptr neighbor_node = node_mgr_->GetNode(neighbor_x, neighbor_y);
            if(nullptr == neighbor_node)
            {
                continue;
            }

            neighbors.push_back(neighbor_node);
        }
    }

    // 路径重构
    PathResult PathFinder::ReconstructPath(GridNode_Ptr end_node)
    {
        if(!end_node)
        {
            return PathResult();
        }

        // 从后往前回溯
        std::vector<GridNode_Ptr> path_result;
        GridNode_Ptr node = end_node;
        while(node)
        {
            path_result.push_back(node);
            node = node->parent_node;
        }

        // 反转路径（从起点到终点）
        std::reverse(path_result.begin(), path_result.end());

        // ************* 生成结果 **************************
        PathResult result;
        for(const auto& path_node : path_result)
        {
            // 网格坐标路径  path
            result.path.push_back(std::pair(path_node->x, path_node->y));

            // 世界坐标路径 world_path
            std::pair<int32_t, int32_t> world_pos = map_->GridToWorld(path_node->x, path_node->y);
            // 如果和前面一个坐标一致，则不放入
            if(false == result.world_path.empty() && result.world_path.back() == world_pos)
            {
                continue;
            }

            result.world_path.push_back(std::pair(path_node->x, path_node->y));
        }

        result.found = true;
        return result;
    }

    // 检查是否可通行
    bool PathFinder::IsWalkable(int32_t x, int32_t y) const
    {
        if((x < 0) || (y < 0) || (nullptr == map_) || (x >= map_->GetWidth()) || (y >= map_->GetHeight()))
        {
            return false;
        }

        return map_->IsWalkable(x, y);
    }
}