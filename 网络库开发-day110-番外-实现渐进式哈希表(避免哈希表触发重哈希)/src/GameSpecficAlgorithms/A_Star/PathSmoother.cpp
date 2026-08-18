#include "PathSmoother.h"

namespace A_Star
{
    // 平滑路径：删除冗余点
    std::vector<PathPoint> PathSmoother::SmoothPath(const std::vector<PathPoint>& path)
    {
        std::vector<PathPoint> result;
        if(path.size() < 3) 
        {
            return path;
        }

        result.push_back(path[0]);
        int path_len = path.size();
        size_t i = 0;
        while(i < path_len -1)
        {
            size_t furthest = i + 1;
            // 从最远的点开始往前检查，第一个可视点就是最远的
            for (size_t j = path_len - 1; j >= (i + 2); --j) 
            {
                if (HasLineOfSight(path[i].grid_x, path[i].grid_y, path[j].grid_x, path[j].grid_y)) {
                    furthest = j;
                    break;
                }
            }

            result.push_back(path[furthest]);
            i = furthest;
        }

        // 确保终点被包含
        PathPoint result_last = result.back();
        PathPoint path_last = path.back();
        if((result_last.grid_x != path_last.grid_x) ||  (result_last.grid_y != path_last.grid_y))
        {
            result.push_back(path.back());
        }

        return result;
    }

    // 检查两点之间是否可直线通行（无障碍物）
    bool PathSmoother::HasLineOfSight(int32_t src_x, int32_t src_y, int32_t target_x, int32_t target_y) const
    {
        if(!map_)
        {
            return false;
        }

        return IsLineWalkable(src_x, src_y, target_x, target_y);
    }

    // Bresenham 直线算法，检查路径上的所有格子
    bool PathSmoother::IsLineWalkable(int32_t src_x, int32_t src_y, int32_t target_x, int32_t target_y) const
    {
        if(!map_)
        {
            return false;
        }

        int32_t dx = std::abs(target_x - src_x);
        int32_t dy = std::abs(target_y - src_y);

        int32_t step_x = (src_x < target_x)? 1 : -1;
        int32_t step_y = (src_y < target_y)? 1 : -1;

        // ===== 经典 Bresenham 误差初始化 =====
        // err 表示"当前路径与理想直线的偏差"
        // 初始值 = 2*dy - dx（当 dx >= dy 时）
        // 这里直接使用完全形式，支持所有方向
        int32_t err = (dx > dy) ? (2 * dy - dx) : (2 * dx - dy);

        int32_t move_x = src_x;
        int32_t move_y = src_y;

        while(true)
        {
           // 检查当前格子是否可行走
           if(false == map_->IsWalkable(move_x, move_y))
           {
                return false;
           } 

           if(move_x == target_x && move_y == target_y)
           {
                break;
           }

           // ===== 经典决策：根据主方向选择步进策略 =====
           if(dx >= dy)
           {
                // 水平方向为主（X 轴是长轴）
                // 每步必定沿 X 方向走一格
                move_x += (step_x);

                // 误差 >= 0 时，Y 方向也需要走一格（斜向）
                if(err >= 0)
                {
                    move_y += (step_y);
                    err -= 2 * dx;   // 修正误差：因为 Y 轴也走了一步
                }

                err += 2 * dy;        // 每走一步 X，误差增加 2*dy
           }
           else
           {
                // 垂直方向为主（Y 轴是长轴）
                // 每步必定沿 Y 方向走一格
                move_y += (step_y);

                 // 误差 >= 0 时，X 方向也需要走一格（斜向）
                if(err >= 0)
                {
                    move_x += (step_x);
                    err -= 2 * dy;   // 修正误差：因为 X 轴也走了一步
                }

                err += 2 * dx;        // 每走一步 Y，误差增加 2*dx
           }

        }

        return true;
    }
}