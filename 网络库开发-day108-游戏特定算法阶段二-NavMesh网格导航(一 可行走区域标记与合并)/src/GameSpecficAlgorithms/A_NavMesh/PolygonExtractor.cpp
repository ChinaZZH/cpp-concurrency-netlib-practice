#include "PolygonExtractor.h"
#include <algorithm>
#include <cmath>
#include <iostream>

namespace NavMesh
{
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////Polygon///////////////////////////////////////////Polygon///////////////////////Polygon/////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    bool Polygon::ContainsPoint(float px, float py) const
    {
        /*
        射线法（Ray Casting Algorithm），用于判断一个点是否在多边形内部。

        1. 核心思想
        从目标点 (px, py) 向正右方画一条水平射线，统计这条射线与多边形边界的交点数：

        奇数个交点 → 点在多边形内部

        偶数个交点 → 点在多边形外部
        */

        if(vertices.size() < 3)
        {
            return false;
        }

        bool inside = false;
        size_t pt_count = vertices.size();
        for(int i = 0; i < pt_count; ++i)
        {
            int next_idx = (i + 1) % pt_count;
            float current_y = vertices[i].y;
            float next_y = vertices[next_idx].y;
            if((current_y > py) != (next_y > py))
            {
                // 利用两点式
                float current_x = vertices[i].x;
                float next_x = vertices[next_idx].x;

                float result_x = current_x + (next_x - current_x) * (py - current_y) / (next_y - current_y);
                if(result_x < px)
                {
                    inside = !inside;
                }
            }
        }

        return inside;
    }

    float Polygon::GetArea() const
    {
        if(vertices.size() < 3)
        {
            return 0.00f;
        }

        // 鞋带公式（Shoelace Formula），也叫高斯面积公式。它的核心思想是：把多边形的面积分解成一系列三角形面积的和
        float area = 0.00f;
        size_t pt_count = vertices.size();
        for(int i = 0; i < pt_count; ++i)
        {
            int next_idx = (i + 1) % pt_count;
            area += (vertices[i].x * vertices[next_idx].y);
            area -= (vertices[next_idx].x * vertices[i].y);
        }

        area = std::abs(area) / 2.0f;
        return area;
    }

    void Polygon::CalculateCenter()
    {
        if(vertices.empty())
        {
            center_x = 0.0f;
            center_y = 0.0f;
            return ;
        }

        float tmp_center_x = 0.0f;
        float tmp_center_y = 0.0f;
        for(const auto& pt : vertices)
        {
            tmp_center_x += (pt.x);
            tmp_center_y += (pt.y);
        }

        center_x = tmp_center_x / vertices.size();
        center_y = tmp_center_y / vertices.size();
    }

    // Polygon 结构体中新增方法
    void Polygon::CalculateCentroid() 
    {
        if (vertices.size() < 3) {
            center_x = 0.0f;
            center_y = 0.0f;
            return;
        }

        float area_sum = 0.0f;
        float cx_sum = 0.0f;
        float cy_sum = 0.0f;
        size_t n = vertices.size();

        // 以顶点 0 为基准，将多边形剖分成三角形 (0, i, i+1)
        for (size_t i = 1; i < n - 1; ++i) 
        {
            const Point2D& p0 = vertices[0];
            const Point2D& p1 = vertices[i];
            const Point2D& p2 = vertices[i + 1];

            // 计算三角形面积（叉积的一半，带符号）
            float area = (p1.x - p0.x) * (p2.y - p0.y) -
                        (p2.x - p0.x) * (p1.y - p0.y);

            // 三角形重心（顶点平均）
            float tri_cx = (p0.x + p1.x + p2.x) / 3.0f;
            float tri_cy = (p0.y + p1.y + p2.y) / 3.0f;

            area_sum += area;
            cx_sum += area * tri_cx;
            cy_sum += area * tri_cy;
        }


        if (std::abs(area_sum) < 0.0001f) 
        {
            // 退化为直线或点，回退到顶点平均
            center_x = 0.0f;
            center_y = 0.0f;
            for (const auto& v : vertices) {
                center_x += v.x;
                center_y += v.y;
            }
        
            center_x /= vertices.size();
            center_y /= vertices.size();
            return;
        }

        // 质心 = 加权平均（面积作为权重）
        center_x = cx_sum / area_sum;
        center_y = cy_sum / area_sum;
    }

    void Polygon::GetBounds(float& min_x, float& min_y, float& max_x, float& max_y) const
    {
        if(vertices.empty())
        {
            min_x = 0.00f;
            min_y = 0.00f;
            max_x = 0.00f;
            max_y = 0.00f;
            return ;
        }

        min_x = vertices[0].x;
        max_x = vertices[0].x;

        min_y = vertices[0].y;
        max_y = vertices[0].y;

        for(const auto& pt : vertices)
        {
            if(pt.x < min_x)
            {
                min_x = pt.x;
            }

            if(pt.x > max_x)
            {
                max_x = pt.x;
            }

            if(pt.y < min_y)
            {
                min_y = pt.y;
            }

            if(pt.y > max_y)
            {
                max_y = pt.y;
            }
        }
    }

    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////PolygonExtractor///////////////////////////////////////////PolygonExtractor///////////////////////PolygonExtractor///
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // 从区域标记网格提取多边形
    // 参数：map, region_ids（子任务1.1的输出）
    // 输出：每个区域的多边形
    std::vector<RegionPolygon> PolygonExtractor::Extract(const std::shared_ptr<A_Star::GridMap> map, const std::vector<uint32_t>& region_ids)
    {
        uint32_t width = map->GetWidth();
        uint32_t height = map->GetHeight();

        // 1. 按区域 ID 分组收集网格索引
        std::unordered_map<uint32_t, std::vector<uint32_t>> region_grid_map;
        for(uint32_t y = 0; y < height; ++y)
        {
            for(uint32_t x = 0; x < width; ++x)
            {
                int index = y * width + x;
                uint32_t region_id = region_ids[index];
                if(0 == region_id)
                {
                    continue;
                }

                std::vector<uint32_t>& vecGridIdx = region_grid_map[region_id];
                vecGridIdx.push_back(index);
            }
        }


        // 2. 为每个区域提取多边形
        std::vector<RegionPolygon> result;
        result.reserve(region_grid_map.size());
        for(const auto& [id, grid_idx_arry] : region_grid_map)
        {
            RegionPolygon region_polygon;
            region_polygon.region_id = id;
            region_polygon.grid_index_arry = grid_idx_arry;

            region_polygon.polygon = this->ExtractPolygonFromRegion(map, id, grid_idx_arry);
            region_polygon.polygon = this->SimplifyPolygon(region_polygon.polygon);
            region_polygon.polygon.CalculateCentroid();
            

            result.emplace_back(region_polygon);
        }


        std::cout << "[PolygonExtractor] Extracted " << result.size() << " polygons" << std::endl;
        return result;
    }


    // 获取属于某个区域的所有网格索引
    std::vector<uint32_t> PolygonExtractor::GetGridIndexArrayForRegion(
            const std::vector<uint32_t>& region_ids, 
            uint32_t region_id,
            uint32_t width,
            uint32_t height)
    {
         std::vector<uint32_t> result;
         for(uint32_t y = 0; y < height; ++y)
         {
            for(uint32_t x = 0; x < width; ++x)
            {
                uint32_t index = y * width + x;
                if(region_ids[index] == region_id)
                {
                    result.push_back(index);
                }
            }
         }

         return result;
    }


    // 提取单个区域的多边形
    Polygon PolygonExtractor::ExtractPolygonFromRegion(
            const std::shared_ptr<A_Star::GridMap> map, 
            uint32_t region_id, 
            const std::vector<uint32_t>& grid_index_arry)
    {
        Polygon result_polygon;
        if(grid_index_arry.empty())
        {
            return result_polygon;
        }

        uint32_t width = map->GetWidth();
        uint32_t height = map->GetHeight();
        
        // 收集区域所有格子坐标
        std::vector<std::pair<uint32_t, uint32_t>> cells;
        for(const auto& idx : grid_index_arry)
        {
            uint32_t x = idx % width;
            uint32_t y = idx / width;
            cells.push_back(std::pair(x, y));
        }

        // 找出边界格子
        std::vector<Point2D> boundary_points;
        std::unordered_set<uint32_t> index_set(grid_index_arry.begin(), grid_index_arry.end());
        for(const auto& cell : cells)
        {
            uint32_t x = cell.first;
            uint32_t y = cell.second;

            bool is_bounary = false;
            is_bounary = this->CheckIsBounary(index_set, x, y, width, height);
            

            if(is_bounary)
            {
                std::pair<int, int> world_position = map->GridToWorld(x, y);

                Point2D point;
                point.x = static_cast<float>(world_position.first);
                point.y = static_cast<float>(world_position.second);
                boundary_points.push_back(point);
            }
        }

        if(boundary_points.size() < 3)
        {
            return result_polygon;
        }

        // 计算中心点
        float center_x = 0.0f;
        float center_y = 0.0f;
        for(const auto& p : boundary_points)
        {
            center_x += p.x;
            center_y += p.y;
        }

        center_x = center_x / boundary_points.size();
        center_y = center_y / boundary_points.size();

        // 按角度排序，形成凸多边形
        std::sort(boundary_points.begin(), boundary_points.end(), [center_x, center_y](const Point2D& prev, const Point2D& next){
            float angle_prev = std::atan2(prev.y - center_y, prev.x - center_x);
            float angle_next = std::atan2(next.y - center_y, next.x - center_x);
            return angle_prev < angle_next;
        });

         // 去重
         auto itr_unique = std::unique(boundary_points.begin(), boundary_points.end());
         if(itr_unique != boundary_points.end())
         {
             boundary_points.erase(itr_unique, boundary_points.end());
         }
        
         result_polygon.vertices = boundary_points;
         result_polygon.center_x = center_x;
         result_polygon.center_y = center_y;
         result_polygon.CalculateCentroid();

         return result_polygon;
    }

    // 简化多边形（去除共线点）
    Polygon PolygonExtractor::SimplifyPolygon(const Polygon& poly)
    {
        if(poly.vertices.size() < 3)
        {
            return poly;
        }

        Polygon result;
        const auto& pts = poly.vertices;
        size_t pt_count = pts.size();

        for(size_t i = 0; i < pt_count; ++i)
        {
            int prev = (i - 1 + pt_count) % pt_count;
            int next = (i + 1) % pt_count;
            const Point2D& point = pts[i];

             // 计算叉积，判断三点是否共线 其实就是算两条线的斜率是否相同，如果不相同说明是拐点
             // [ (next_y - y) / (next_x - x) ] ==  [ (y - prev_y) / (x - pre_x) ] 可以推导出
             // [ (next_y - y) *(x - pre_x) ] ==  [ (y - prev_y) *(next_x - x)  ] 可以推导出
            float left_result =  (pts[next].y - point.y) * (point.x - pts[prev].x);
            float right_result = (point.y - pts[prev].y) * (pts[next].x - point.x);
            float cross_result = std::abs(left_result - right_result);
            if(cross_result > 0.01f)
            {
                result.vertices.push_back(point);
            }
        }

        result.CalculateCentroid();
        return result;
    }

    // 检查点是否在多边形内（射线法）
    bool PolygonExtractor::PointInPolygon(const std::vector<Point2D>& vertices, float px, float py) const
    {
        Polygon tmp;
        tmp.vertices = vertices;
        return tmp.ContainsPoint(px, py);
    }


    bool PolygonExtractor::CheckIsBounary(const std::unordered_set<uint32_t>& index_set, uint32_t x, uint32_t y, uint32_t width, uint32_t height)
    {
        // 一次检查四个方向

        // 检查上方
        {
            if(0 == y)
            {
                return true;
            }

            int index = (y - 1) * width + x;
            auto itr_check = index_set.find(index);
            if(itr_check == index_set.end())
            {
                return true;
            }

        }
        
        // 检查下放
        {
            if(y >= height -1)
            {
                return true;
            }

            int index = (y + 1) * width + x;
            auto itr_check = index_set.find(index);
            if(itr_check == index_set.end())
            {
                return true;
            }
        }

        // 检查左边
        {
            if(0 == x)
            {
                return true;
            }

            int index = y * width + x - 1;
            auto itr_check = index_set.find(index);
            if(itr_check == index_set.end())
            {
                return true;
            }
        }

        // 检查右边
        {
            if(x >= width-1)
            {
                return true;
            }

            int index = y * width + x + 1;
            auto itr_check = index_set.find(index);
            if(itr_check == index_set.end())
            {
                return true;
            }
        }

        return false;
    }
}