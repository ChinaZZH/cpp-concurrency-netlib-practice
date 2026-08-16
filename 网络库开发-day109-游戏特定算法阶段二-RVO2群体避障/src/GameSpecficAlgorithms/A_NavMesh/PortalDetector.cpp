#include "PortalDetector.h"
#include <iostream>
#include <algorithm>
#include <unordered_set>

namespace NavMesh
{
    PortalDetector::PortalDetector(const std::vector<RegionPolygon>& regions, 
            const std::vector<uint32_t>& region_ids, 
            const std::shared_ptr<A_Star::GridMap> map
        ):regions_(regions), region_ids_(region_ids), map_(map)
    {
        for(uint32_t i = 0; i < region_ids.size(); ++i)
        {
            uint32_t id = region_ids[i];
            region_index_map_[id] = i; 
        }
    }

    // 检测所有门户
    void PortalDetector::DetectPortals()
    {
        portals_.clear();
        DetectCandidatesAndPortals();
        std::cout << "[PortalDetector] Found " << portals_.size() << " portals." << std::endl;
    }


    // 构建连接图（region_id → 相邻区域 ID 列表）
    std::unordered_map<uint32_t, std::vector<uint32_t>> PortalDetector::BuildConnectionGraph() const
    {
        std::unordered_map<uint32_t, std::vector<uint32_t>> graph;
        for(const auto& protal: portals_)
        {
            graph[protal.region_id_first].push_back(protal.region_id_second);
            graph[protal.region_id_second].push_back(protal.region_id_first);
        }

        // 去重每个区域的邻居列表
        for(auto& [region_id, neighbor_region] : graph)
        {
            std::sort(neighbor_region.begin(), neighbor_region.end());
            auto itr_erase = std::unique(neighbor_region.begin(), neighbor_region.end());
            if(itr_erase != neighbor_region.end())
            {
                neighbor_region.erase(itr_erase, neighbor_region.end());
            }
        }

        return graph;
    }

    // 核心方法: 遍历网格边界， 找到所有候选邻居对， 并检测门户
    void PortalDetector::DetectCandidatesAndPortals()
    {
       uint32_t width = map_->GetWidth();
       uint32_t height = map_->GetHeight();

       // 使用集合去重候选邻居对
       auto Make_Edge_Key = [](uint32_t a, uint32_t b) -> uint64_t {
            if(a > b)
            {
                uint32_t temp = a;
                a = b;
                b = temp;
            }

            return (static_cast<uint64_t>(a) << 32) | b;
       };

       
       //  找到边界格子 → 穿过它旁边的墙 → 找到墙后的另一个区域 → 验证它们之间是否有可通行的门 → 记录下来。
       //  墙的厚度被隐含地限制为 1 格, 如果以后需要更厚或者么有限制的墙体，那个时候再做调整。
       std::unordered_set<uint64_t> candidate_set;
       auto Local_Process_Grid_Index = [this, Make_Edge_Key, &candidate_set](uint32_t x, uint32_t y, uint32_t current_region_id){
            int width = map_->GetWidth();
            int height = map_->GetHeight();

            // 检查四个方向，如果某个方向不可通行，则当前格子是边界格子
            const int dx[4] = { 1,  0,  -1,  0 };
            const int dy[4] = { 0,  1,  0,  -1 };
            
            for(int dir = 0; dir < 4; ++dir)
            {
                int nx = static_cast<int>(x) + dx[dir];
                int ny = static_cast<int>(y) + dy[dir];
                
                if(nx < 0 || ny < 0 || nx >= width || ny >= height) 
                {
                    continue;
                }

                // 找到边界格子, 需要判断x,y的旁边是否是墙体。 如果可通行说明不是墙体，则继续循环 
                if(map_->IsWalkable(nx, ny))
                {
                    continue;
                }

                // nx,ny 是墙体则继续判断墙后的另一个区域
                int next_nx = nx + dx[dir];
                int next_ny = ny + dy[dir];
                if(next_nx < 0 || next_ny < 0 || next_nx >= width || next_ny >= height)
                {
                    continue;
                }
                
                // 墙后仍然不是可移动区域
                if(false == map_->IsWalkable(next_nx, next_ny))
                {
                    continue;
                }

                int next_index = next_ny * width + next_nx;
                int next_region_id = region_ids_[next_index];
                if(0 == next_region_id || next_region_id == current_region_id)
                {
                    continue;
                }

                // 判断是否已经在临接图中
                uint64_t key = Make_Edge_Key(current_region_id, next_region_id);
                auto itr = candidate_set.find(key);
                if(itr != candidate_set.end())
                {
                    continue;
                }


                candidate_set.insert(key);
                // 尝试检测门户
                Portal portal;
                if(FindPortalBetween(current_region_id, next_region_id, portal))
                {
                    portals_.push_back(portal);
                }
            }
       };

        // 遍历所有格子，寻找边界格子（可通行格子旁边有不可通行格子）
       for(uint32_t y = 0; y < height; ++y)
       {
            for(uint32_t x = 0; y < width; ++y)
            {
                if(false == map_->IsWalkable(x, y))
                {
                    continue;
                }

                uint32_t index = y * width + x;
                uint32_t current_region_id = region_ids_[index];
                if(0 == current_region_id)
                {
                    continue;
                }

                // 处理单个各自的情况
                Local_Process_Grid_Index(x, y, current_region_id);
            }
       }
    }


    // 对某一对区域检测是否存在可通行缺口
    bool PortalDetector::FindPortalBetween(uint32_t region_a, uint32_t region_b, Portal& out_portal) const
    {
        // 首先找到两个区域的共享边界中点
        Point2D mid_point;
        if(!FindSharedEdgeMidpoint(region_a, region_b, mid_point))
        {
            return false;
        }

        // 在共享边界上检测可通行缺口
        Point2D center;
        float width;
        if(!FindPassableGap(region_a, region_b, center, width))
        {
            return false;
        }

        out_portal.region_id_first = region_a;
        out_portal.region_id_second = region_b;
        out_portal.center = center;
        out_portal.width = width;
        return true;
    }

    // 辅助：获取网格 (x,y) 的区域 ID（0 表示不可行走）
    uint32_t PortalDetector::GetRegionId(int x, int y) const
    {
        if(x < 0 || y < 0)
        {
            return 0;
        }

        if(x >= map_->GetWidth() || y >= map_->GetHeight())
        {
            return 0;
        }

        uint32_t index = y * map_->GetWidth() + x;
        return region_ids_[index];
    }

    // 辅助：计算两个区域共享边界的中点
    bool PortalDetector::FindSharedEdgeMidpoint(uint32_t region_a, uint32_t region_b, Point2D& mid_point) const
    {
        // 获取两个区域的索引
        int a_region_index = -1;
        int b_region_index = -1;
        {
            auto itr_region_a = region_index_map_.find(region_a);
            if(itr_region_a == region_index_map_.end())
            {
                return false;
            }
            
            auto itr_region_b = region_index_map_.find(region_b);
            if(itr_region_b == region_index_map_.end())
            {
                return false;
            }

            a_region_index = (itr_region_a->second);
            b_region_index = (itr_region_b->second);
            if(a_region_index < 0 || a_region_index >= regions_.size() || b_region_index < 0 || b_region_index >= regions_.size())
            {
                return false;
            }
        } 

        const Polygon& polygon_a = regions_[a_region_index].polygon;
        const Polygon& polygon_b = regions_[b_region_index].polygon;
        const float EPSILON = 0.1f; // 由于网格坐标是整数，阈值可以设小

        // 遍历 poly_a 的每条边
        for(size_t i = 0; i < polygon_a.vertices.size(); ++i)
        {
            size_t edge_idx = (i + 1) % polygon_a.vertices.size();
            const Point2D& a_first_point = polygon_a.vertices[i];
            const Point2D& a_second_point = polygon_a.vertices[edge_idx];
            
            Point2D a_mid_point = { (a_first_point.x + a_second_point.x) * 0.5f, (a_first_point.y + a_second_point.y) * 0.5f };
            for(size_t j = 0; j < polygon_b.vertices.size(); ++j)
            {
                size_t next_edge_idx = (j + 1) % polygon_b.vertices.size();
                const Point2D& b_first_point = polygon_b.vertices[j];
                const Point2D& b_second_point = polygon_b.vertices[next_edge_idx];

                // 检查两条边是否重合 简单方法：检查中点是否非常接近）
                Point2D b_mid_point = { (b_first_point.x + b_second_point.x) * 0.5f, (b_first_point.y + b_second_point.y) * 0.5f };
                float dx = a_mid_point.x - b_mid_point.x;
                float dy = a_mid_point.y - b_mid_point.y;
                if((dx*dx + dy*dy) < (EPSILON*EPSILON))
                {
                    mid_point = a_mid_point;
                    return true;
                }
            }
        }

        

        return false;
    }

    // 辅助：在共享边界上检测可通行缺口
    bool PortalDetector::FindPassableGap(uint32_t region_a, uint32_t region_b, Point2D& center, float& width) const
    {
        // 获取两个区域的索引
        int a_region_index = -1;
        int b_region_index = -1;
        {
            auto itr_region_a = region_index_map_.find(region_a);
            if(itr_region_a == region_index_map_.end())
            {
                return false;
            }
            
            auto itr_region_b = region_index_map_.find(region_b);
            if(itr_region_b == region_index_map_.end())
            {
                return false;
            }

            a_region_index = (itr_region_a->second);
            b_region_index = (itr_region_b->second);
            if(a_region_index < 0 || a_region_index >= regions_.size() || b_region_index < 0 || b_region_index >= regions_.size())
            {
                return false;
            }
        } 

        const Polygon& polygon_a = regions_[a_region_index].polygon;
        const Polygon& polygon_b = regions_[b_region_index].polygon;
        
        // 简化实现：找到共享边的中点作为门户中心，宽度取固定值 1.0（单位长度）
        // 真实实现应该检测共享边上可通行的连续缺口长度
        Point2D mid_point;
        if(false == this->FindSharedEdgeMidpoint(region_a, region_b, mid_point))
        {
            return false;
        }

        // 检查该中点是否是可通行的（即没有障碍物）
        // 将中点转换为网格坐标
        std::pair<int, int> grid_pos = map_->WorldToGrid(mid_point.x, mid_point.y);
        if(false == map_->IsWalkable(grid_pos.first, grid_pos.second))
        {
            // 如果中点本身不可通行，可能是误差，尝试微调
            // 简单微调：沿法线方向移动一小步
            // 这里略过，直接返回失败
            return false;
        }

        center = mid_point;
        width = 1.0f; // 简化值
        return true;
    }
}