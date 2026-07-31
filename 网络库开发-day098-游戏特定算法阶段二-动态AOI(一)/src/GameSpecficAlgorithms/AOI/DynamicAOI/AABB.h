#pragma once

struct AABB
{
    float min_x;
    float min_y;
    float max_x;
    float max_y;

    float GetWidth() const  { return max_x - min_x; }
    float GetHeight() const { return max_y - min_y; }
    float GetArea() const { return GetWidth()*GetHeight(); }

    // 判断点是否在区域内
    bool Contains(float px, float py) const
    {
        return (px >= min_x && px <= max_x) && (py >= min_y && py <= max_y);
    }

    // 判断两个区域是否相邻（用于合并检查）
    bool IsAdjacentTo(const AABB& other) const 
    {
        return (min_x == other.max_x || max_x == other.min_x) 
        || (min_y == other.max_y || max_y == other.min_y);
    }

    // 生成四个子区域（四叉树分裂）
    std::vector<AABB> Split() const
    {
        float mid_x = (mid_x + max_x) / 2.0f;
        float mid_y = (mid_y + max_y) / 2.0f;
        return {
            { min_x, min_y, mid_x, mid_y },     // 左下
            { mid_x, min_y, max_x, mid_y },     // 右下
            { min_x, mid_y, mid_x, max_y },     // 左上
            { mid_x, mid_y, max_x, max_y },     // 右上
        };
    }
};