#pragma once
#include <algorithm>
#include <cstdint>
#include <cmath>

namespace RVO2
{
    struct Point2D
    {
        float x;
        float y;

        Point2D()
        :x(0.0f), y(0.0f)
        { }

        Point2D(float tmp_x, float tmp_y)
        :x(tmp_x), y(tmp_y) {}


        bool operator==(const Point2D& other) const  {
            return std::abs(x - other.x) < 0.001f && std::abs(y - other.y) < 0.001f;
        }

        bool operator!=(const Point2D& other) const {
            return !(*this == other);
        }
    };

    struct Agent
    {
        uint32_t    id;                     // Agent 唯一 ID
        Point2D     position;               // 当前位置
        Point2D     velocity;               // 当前速度
        Point2D     target;                 // 目标位置
        float       radius = 0.5f;          // 碰撞半径
        float       max_speed = 0.1f;       // 最大速度（单位/毫秒）
        float       max_accel = 0.05f;      // 最大加速度
        bool        enabled = true;         // 是否参与模拟

    };
}