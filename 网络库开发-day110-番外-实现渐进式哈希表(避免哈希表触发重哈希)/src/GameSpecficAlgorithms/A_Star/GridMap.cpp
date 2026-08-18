#include "GridMap.h"
#include <stdexcept>
#include <iostream>

namespace A_Star
{
    GridMap::GridMap(uint32_t width, uint32_t height, uint32_t cell_size)
    {
        Init(width, height, cell_size);
    }

    // 初始化地图
    void GridMap::Init(uint32_t width, uint32_t height, uint32_t cell_size)
    {
        if (width == 0 || height == 0 || cell_size <= 0) 
        {
            throw std::invalid_argument("Invalid map dimensions");
        }

        this->width_ = width;
        this->height_ = height;
        this->cell_size_ = cell_size;
        cells_.assign(width * height, GridCell{});
    }
        
    
    void GridMap::Clear()
    {
        this->width_ = 0;
        this->height_ = 0;
        this->cell_size_ = 1;
        cells_.clear();
    }

    
    // 查询
    bool GridMap::IsWalkable(uint32_t x, uint32_t y) const
    {
        if(false == this->IsValid(x, y))
        {
            return false;
        }

        int index = y * width_ + x;
        return cells_[index].IsWalkable();
    }

        
    bool GridMap::IsValid(uint32_t x, uint32_t y) const
    {
        if(x >= width_ || y >= height_)
        {
            return false;
        }

        return true;
    }


    // 世界坐标 <-> 网格坐标 转换
    std::pair<int, int> GridMap::WorldToGrid(int x, int y) const
    {
        int gridX = x / cell_size_;
        int gridY = y / cell_size_;

        //std::cout << "GridMap::WorldToGrid_1111 grid_x:=" << gridX << " grid_y:=" << gridY << std::endl;

        // 边界裁剪
        gridX = std::min(static_cast<int>(width_-1), std::max(0, gridX));
        gridY = std::min(static_cast<int>(height_-1), std::max(0, gridY));
        //std::cout << "GridMap::WorldToGrid_2222 grid_x:=" << gridX << " grid_y:=" << gridY << std::endl;
        return std::pair(gridX, gridY);
    }


    std::pair<int, int> GridMap::GridToWorld(int gridX, int gridY) const
    {
        // 取中心点
        int world_middle_x = gridX * cell_size_ + cell_size_ / 2;
        int world_middle_y = gridY * cell_size_ + cell_size_ / 2;
        return std::pair(world_middle_x, world_middle_y);
    }


    // 修改地图
    void GridMap::SetWalkable(uint32_t x, uint32_t y, bool walkable)
    {
        TerrainType terrain = TerrainType::Walkable;
        if(false == walkable)
        {
            terrain = TerrainType::Obstacle;
        }

        SetTerrain(x, y, terrain);
    }


    void GridMap::SetTerrain(uint32_t x, uint32_t y, TerrainType terrain)
    {
        if(false == this->IsValid(x, y))
        {
            return ;
        }

        int index = y * width_ + x;
        cells_[index].terrain = terrain;
    }

    // 加载 / 导出（可选）
    bool GridMap::LoadFromArray(const std::vector<std::vector<bool>>& walkable_map)
    {
        if(walkable_map.empty())
        {
            return false;
        }

        uint32_t width = walkable_map.size();
        uint32_t hegiht = walkable_map[0].size();
        uint32_t cell_size = cell_size_;
        if(width_ <= 0 || height_ <= 0 || cell_size <= 0)
        {
            return false;
        }

        this->Clear();
        this->Init(width, hegiht, cell_size);
        for(int y = 0; y < hegiht; y += 1)
        {
            for(int x = 0; x < width; x += 1)
            {
                if(x >= walkable_map[y].size())
                {
                    continue;
                }

                this->SetWalkable(x, y, walkable_map[y][x]);
            }
        }

        return true;
    }

    std::vector<std::vector<bool>> GridMap::ExportWalkable() const
    {
        std::vector<std::vector<bool>> result(height_, std::vector<bool>(width_, true));
        for(int y = 0; y < height_; ++y)
        {
            for(int x = 0; x < width_; ++x)
            {
                result[y][x] = IsWalkable(x, y);
            }
        }

        return result;
    }
}