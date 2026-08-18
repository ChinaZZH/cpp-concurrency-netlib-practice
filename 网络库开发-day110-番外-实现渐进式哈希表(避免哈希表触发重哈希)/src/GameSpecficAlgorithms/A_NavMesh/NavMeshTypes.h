#pragma once


#include <vector>
#include <cstdint>
#include <cmath>

namespace NavMesh
{
    struct Point2D
    {
        float x;
        float y;

        bool operator==(const Point2D& other) const  {
            return std::abs(x - other.x) < 0.001f && std::abs(y - other.y) < 0.001f;
        }

        bool operator!=(const Point2D& other) const {
            return !(*this == other);
        }
    };

    struct Polygon
    {
        std::vector<Point2D> vertices;
        float center_x = 0.0f;
        float center_y = 0.0f;

        bool ContainsPoint(float px, float py) const;

        float GetArea() const;

        void CalculateCenter();

        void CalculateCentroid();
        
        void GetBounds(float& min_x, float& min_y, float& max_x, float& max_y) const;
    };


    struct RegionPolygon
    {
        uint32_t region_id;
        Polygon polygon;
        std::vector<uint32_t> grid_index_arry; // 该区域包含的网格索引
    };
}