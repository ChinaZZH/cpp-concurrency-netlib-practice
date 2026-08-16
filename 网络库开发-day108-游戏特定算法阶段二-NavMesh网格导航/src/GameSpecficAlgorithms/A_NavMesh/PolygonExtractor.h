#pragma once

#include <vector>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include "NavMeshTypes.h"
#include "../A_Star/GridMap.h"

namespace NavMesh
{


    class PolygonExtractor
    {
    public:
        // 从区域标记网格提取多边形 + 构建邻接图
        // 参数：map, region_ids（子任务1.1的输出）
        // 输出：每个区域的多边形
        std::vector<RegionPolygon> ExtractPolygons(const std::shared_ptr<A_Star::GridMap> map, const std::vector<uint32_t>& region_ids);

    private:
        // 获取属于某个区域的所有网格索引
        std::vector<uint32_t> GetGridIndexArrayForRegion(
            const std::vector<uint32_t>& region_ids, 
            uint32_t region_id,
            uint32_t width,
            uint32_t height
        );

        // 提取单个区域的多边形
        Polygon ExtractPolygonFromRegion(
            const std::shared_ptr<A_Star::GridMap> map, 
            uint32_t region_id, 
            const std::vector<uint32_t>& grid_index_arry
        ); 

        // 简化多边形（去除共线点）
        Polygon SimplifyPolygon(const Polygon& poly);

        // 检查点是否在多边形内（射线法）
        bool PointInPolygon(const std::vector<Point2D>& vertices, float px, float py) const;

        // 判断是否处于边界中
        bool CheckIsBounary(const std::unordered_set<uint32_t>& index_set, uint32_t x, uint32_t y, uint32_t width, uint32_t height);

    };
}