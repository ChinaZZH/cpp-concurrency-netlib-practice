#include "RegionMarker.h"
#include <iostream>
#include <unordered_map>

namespace NavMesh
{
    /*
    1. 遍历所有格子，只处理可行走格子

    2. 检查当前格子的上方和左方邻居：

        2.1. 如果上方和左方都不可行走 → 新区域

        2.2. 如果只有上方可行走 → 继承上方的区域 ID

        2.3. 如果只有左方可行走 → 继承左方的区域 ID

        2.4. 如果上方和左方都可行走但区域 ID 不同 → 合并两个区域

    3. 第二次遍历，统一区域 ID（路径压缩）
    */

    // 输入：GridMap，输出：每个格子的区域 ID（0 表示不可行走）
    std::vector<uint32_t> RegionMarker::MarkRegions(const std::shared_ptr<A_Star::GridMap> map)
    {
        region_count_ = 0;
        
        uint32_t width = map->GetWidth();
        uint32_t height = map->GetHeight();
        std::vector<uint32_t> region_ids(width * height, 0);

        // 并查集
        std::vector<uint32_t> parent;
        parent.push_back(0);

        
        auto FindRoot = [&](uint32_t x) {
            uint32_t root = x;
            while(parent[root] != root)
            {
                root = parent[root];
            }

            // 缩短路径
            while(parent[x] != x)
            {
                int next = parent[x];
                parent[x] = root;
                x = next;
            }

            return root;
        };


        auto MergeRegionId = [&](uint32_t b, uint32_t a) {
            uint32_t root_a = FindRoot(a);
            uint32_t root_b = FindRoot(b);
            if(root_a != root_b)
            {
                parent[root_b] = root_a;
            }
        };

        // 第一次遍历：分配区域 ID
        for(uint32_t y = 0; y < height; ++y)
        {
            for(uint32_t x = 0; x < width; ++x)
            {
                // 1. 遍历所有格子，只处理可行走格子
                if(false == map->IsWalkable(x, y))
                {
                    continue;
                }

                //  2. 检查当前格子的上方和左方邻居：
                //  2.1. 如果上方和左方都不可行走 → 新区域
                uint32_t idx = y * width + x;

                // 在屏幕/图像坐标系中： y = 0 在顶部，y 增大方向是向下 所以「上方」是 y - 1，「下方」是 y + 1
                uint32_t up_idx = (y > 0) ? ((y-1)*width + x) : 0;
                uint32_t left_idx = (x > 0) ?  (y*width + x - 1) : 0;

                bool up_walkable = false;
                if(y > 0)
                {
                    up_walkable = map->IsWalkable(x, y - 1);
                }

                bool left_walkable = false;
                if(x > 0)
                {
                    left_walkable = map->IsWalkable(x - 1, y);
                }

                // 2.1. 如果上方和左方都不可行走 → 新区域
                if(false == up_walkable && false == left_walkable)
                {
                    region_count_ += 1;
                    parent.push_back(region_count_);
                    region_ids[idx] = region_count_;
                }
                else if(up_walkable && false == left_walkable)
                {
                    // 2.2. 如果只有上方可行走 → 继承上方的区域 ID
                    region_ids[idx] = region_ids[up_idx];
                }
                else if(false == up_walkable && left_walkable)
                {
                    // 2.3. 如果只有左方可行走 → 继承左方的区域 ID
                    region_ids[idx] = region_ids[left_idx];
                }
                else
                {
                    // 2.4. 如果上方和左方都可行走但区域 ID 不同 → 合并两个区域
                    uint32_t up_region_id = region_ids[up_idx];
                    uint32_t left_region_id = region_ids[left_idx];
                    if(up_region_id == left_region_id)
                    {
                        region_ids[idx] = up_region_id;
                    }
                    else
                    {
                        region_ids[idx] = up_region_id;
                        MergeRegionId(up_region_id, left_region_id);
                    }
                }
            }
        }

        // 第二次遍历：统一区域 ID（路径压缩）
        for(uint32_t y = 0; y < height; ++y)
        {
            for(uint32_t x = 0; x < width; ++x)
            {
                if(false == map->IsWalkable(x, y))
                {
                    continue;
                }

                uint32_t idx = y * width + x;
                uint32_t region_id = region_ids[idx];
                if(region_id > 0)
                {
                    region_ids[idx] = FindRoot(region_id);
                }
            }
        }

        // 重新映射为连续的区域 ID
        int new_next_regino_id = 0;
        std::unordered_map<uint32_t, uint32_t> remap;
        for (uint32_t y = 0; y < height; ++y) 
        {
            for (uint32_t x = 0; x < width; ++x) 
            {
                if (!map->IsWalkable(x, y)) 
                {
                    continue;
                }

                uint32_t idx = y * width + x;
                uint32_t id = region_ids[idx];
                if(id > 0 && remap.find(id) == remap.end()) 
                {
                    new_next_regino_id += 1;
                    remap[id] = new_next_regino_id;
                }
            }
        }

        region_count_ = new_next_regino_id;
        for (uint32_t y = 0; y < height; ++y) 
        {
            for (uint32_t x = 0; x < width; ++x) 
            {
                uint32_t idx = y * width + x;
                uint32_t old_region_id = region_ids[idx];
                if((old_region_id != 0) && (remap.find(old_region_id) != remap.end()))
                {
                    region_ids[idx] = remap[old_region_id];
                }
            }
        }
   

        std::cout << "[RegionMarker] Found " << region_count_ << " regions" << std::endl;
        return region_ids;
    }


}