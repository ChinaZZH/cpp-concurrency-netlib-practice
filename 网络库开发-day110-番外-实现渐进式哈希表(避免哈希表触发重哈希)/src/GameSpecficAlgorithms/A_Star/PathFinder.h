#pragma once
#include <vector>
#include <cstdint>
#include <functional>
#include <memory>
#include "GridNode.h"
#include "GridMap.h"
#include "NodeManager.h"

namespace A_Star
{
    struct PathResult
    {
        bool found = false;
        std::vector<std::pair<int32_t, int32_t>> path;          // 网格坐标的路径
        std::vector<std::pair<int32_t, int32_t>> world_path;    // 世界坐标的路径
        uint32_t nodes_explored = 0;
        float total_cost = 0.00f;
    };

    // 启发式的选择
    enum class HeuristicType
    {
        Manhattan,   // （曼哈顿距离）|dx| + |dy|（只能上下左右走，不能斜穿）
        Euclidean,   // （欧几里得距离）	sqrt(dx² + dy²)（可沿任意角度直线走）
        Diagonal,    //  (对角线距离）	min(dx, dy) * sqrt(2) + |dx - dy|（允许走斜线，但斜线代价是 1.414）
    };

    class PathFinder
    {
    public:
        PathFinder(std::shared_ptr<GridMap> map, std::shared_ptr<NodeManager> node_mgr);

        // 设置启发式类型（默认曼哈顿）
        void SetHeuristic(HeuristicType type) { heuristic_type_ = type; }

        // 设置移动代价权重（可选）
        void SetDiagonalCost(float cost) { diagonal_cost_ = cost; }

        // 核心寻路接口
        PathResult FindPath(int32_t start_x, int32_t start_y, int32_t end_x, int32_t end_y, bool allow_diagonal = false);

        // 取消正在进行的寻路（用于分帧寻路，暂未实现）
        void Cancel() {  canceled_ = true; }

    private:
        // 启发式函数
        float Heuristic(int32_t src_x, int32_t src_y, int32_t target_x, int32_t target_y) const;

        // 邻居生成
        void GetNeighbors(GridNode_Ptr node, std::vector<GridNode_Ptr>& neighbors, bool allow_diagonal) const;

        // 路径重构
        PathResult ReconstructPath(GridNode_Ptr end_node);

        // 检查是否可通行
        bool IsWalkable(int32_t x, int32_t y) const;

    private:
        // 成员变量
        std::shared_ptr<GridMap> map_; 
        
        std::shared_ptr<NodeManager> node_mgr_;
        
        HeuristicType heuristic_type_ = HeuristicType::Manhattan;
        
        float diagonal_cost_ = 1.414f; // sqrt(2)

        bool canceled_ = false; 
    };
}