#pragma once

#include <vector>
#include <cstdint>
#include "../../Common/AABB.h"

namespace A_Star
{
    enum class TerrainType : uint8_t {
        Walkable,           // 可行走的地面     允许通行，移动代价为 1.0（基准值）
        Obstacle,           // 障碍物           禁止通行，A* 不会将其加入开放列表
        Water,              // 水域（河流、湖泊、沼泽）     允许通行，但移动代价更高（如 3.0 ~ 5.0）
        Cliff,              // 悬崖/陡坡        通常禁止通行（或需特殊条件，如飞行单位可通行）
    };


    struct GridCell {
        TerrainType terrain = TerrainType::Walkable;
        bool IsWalkable() const { return TerrainType::Walkable == terrain; }
    };


    class GridMap
    {
    public:
        GridMap() = default;

        GridMap(uint32_t width, uint32_t height, uint32_t cell_size);

        // 初始化地图
        void Init(uint32_t width, uint32_t height, uint32_t cell_size);
        void Clear();

        // 查询
        bool IsWalkable(uint32_t x, uint32_t y) const;

        bool IsValid(uint32_t x, uint32_t y) const;

        uint32_t GetWidth() const { return width_; }

        uint32_t GetHeight() const { return height_; }

        uint32_t GetCellSize() const { return cell_size_; }

        // 世界坐标 <-> 网格坐标 转换
        std::pair<int, int> WorldToGrid(int x, int y) const;

        std::pair<int, int>  GridToWorld(int gridX, int gridY) const;


        // 修改地图
        void SetWalkable(uint32_t x, uint32_t y, bool walkable);

        void SetTerrain(uint32_t x, uint32_t y, TerrainType terrain);

        // 加载 / 导出（可选）
        bool LoadFromArray(const std::vector<std::vector<bool>>& Walkable_map);

        std::vector<std::vector<bool>> ExportWalkable() const;

    private:
        uint32_t width_         = 0;
        uint32_t height_        = 0;
        int cell_size_          = 1;
        std::vector<GridCell>   cells_;
    };
}