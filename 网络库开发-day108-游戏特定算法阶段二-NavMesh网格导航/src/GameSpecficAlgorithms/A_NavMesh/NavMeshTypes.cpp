#include "NavMeshTypes.h"
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
}