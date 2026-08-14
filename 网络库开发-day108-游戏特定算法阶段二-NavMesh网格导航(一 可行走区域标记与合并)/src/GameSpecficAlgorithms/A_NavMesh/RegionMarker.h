#pragma once

#include <vector>
#include <cstdint>
#include <memory>
#include "../A_Star/GridMap.h"

namespace NavMesh
{
    class RegionMarker
    {
    public:
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
        std::vector<uint32_t> MarkRegions(const std::shared_ptr<A_Star::GridMap> map);

        // 获取区域数量
        uint32_t GetRegionCount() const { return region_count_; }

    private:
        uint32_t region_count_ = 0;
    };
}