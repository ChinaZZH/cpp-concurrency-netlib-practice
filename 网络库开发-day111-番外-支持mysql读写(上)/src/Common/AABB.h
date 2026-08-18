#pragma once

#include <vector>

// AABB（轴对齐包围盒）
// 轴对齐（Axis-Aligned）：矩形的边永远平行于 X 轴和 Y 轴，不会旋转。
// 包围盒（Bounding Box）：用来“框住”某个区域或物体

struct AABB
{
    int min_x;
    int min_y;
    int max_x;
    int max_y;

    
    int GetWidth() const  { return max_x - min_x; }
    int GetHeight() const { return max_y - min_y; }
    int GetArea() const { return GetWidth()*GetHeight(); }

    // 判断点是否在区域内
    bool Contains(int px, int py) const
    {
        return (px >= min_x && px <= max_x) && (py >= min_y && py <= max_y);
    }

    bool IsContain(const AABB& containAABB) const 
    {
        return (containAABB.min_x >= min_x && containAABB.min_x <= max_x)
        && (containAABB.max_x >= min_x && containAABB.max_x <= max_x)
        && (containAABB.min_y >= min_y && containAABB.min_y <= max_y)
        && (containAABB.max_y >= min_y && containAABB.max_y <= max_y);
    }

    // 判断两个区域是否相邻（用于合并检查）
    bool IsAdjacentTo(const AABB& other) const 
    {
        // 相邻边为 y轴
        if((min_x == other.max_x || max_x == other.min_x) && (min_y == other.min_y && max_y == other.max_y))
        {
            return true;
        }

        // 相邻边为 x轴
        if((min_y == other.max_y || max_y == other.min_y) && (min_x == other.min_x && max_x == other.max_x))
        {
            return true;
        }

        return false;
    }

    bool Merge(const AABB& other)
    {
        if(false == IsAdjacentTo(other))
        {
            return false;
        }

        min_x = std::min(min_x, other.min_x);
        max_x = std::max(max_x, other.max_x);
        min_y = std::min(min_y, other.min_y);
        max_y = std::max(max_y, other.max_y);
        return true;
    }

    // 生成四个子区域（四叉树分裂）
    std::vector<AABB> Split() const
    {
        int mid_x = (min_x + max_x) / 2.0f;
        int mid_y = (min_y + max_y) / 2.0f;
        return {
            { min_x, min_y, mid_x, mid_y },     // 左下
            { mid_x, min_y, max_x, mid_y },     // 右下
            { min_x, mid_y, mid_x, max_y },     // 左上
            { mid_x, mid_y, max_x, max_y },     // 右上
        };
    }

    // 判断两个区域是否相交
    bool IsIntersectWith(const AABB& other) const 
    {
        return (min_x <= other.max_x && max_x >= other.min_x) &&
               (min_y <= other.max_y && max_y >= other.min_y);
    }
};