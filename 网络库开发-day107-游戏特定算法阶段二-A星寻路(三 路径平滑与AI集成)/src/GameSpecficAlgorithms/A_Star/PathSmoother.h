#pragma once
#include <vector>
#include <cstdint>
#include <memory>
#include "GridMap.h"

namespace A_Star
{
    struct PathPoint
    {
        int32_t grid_x;
        int32_t grid_y;
        float world_x;
        float world_y;
    };


    class PathSmoother
    {
    public:
        explicit PathSmoother(std::shared_ptr<GridMap> map)
        :map_(map)
        { }

        // 平滑路径：删除冗余点
        std::vector<PathPoint> SmoothPath(const std::vector<PathPoint>& path);

        // 设置平滑强度
        void SetSmoothStrength(float smooth_strength) { smooth_strength_ = smooth_strength; }

        // 检查两点之间是否可直线通行（无障碍物）
         bool HasLineOfSight(int32_t src_x, int32_t src_y, int32_t target_x, int32_t target_y) const;

    private:
         // Bresenham 直线算法，检查路径上的所有格子
         bool IsLineWalkable(int32_t src_x, int32_t src_y, int32_t target_x, int32_t target_y) const;

    private:
        std::shared_ptr<GridMap> map_;
        float smooth_strength_ = 1.0f;
    };
}