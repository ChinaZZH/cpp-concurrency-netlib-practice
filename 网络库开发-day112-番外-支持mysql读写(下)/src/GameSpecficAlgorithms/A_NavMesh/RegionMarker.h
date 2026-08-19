#pragma once

#include <vector>
#include <cstdint>
#include <memory>
#include "../A_Star/GridMap.h"

namespace NavMesh
{
    enum class RegionMode
    {
        Connectivity,       // 基于连通性合并（默认）
        Grid,               // 按固定网格划分
    };

    class RegionMarker
    {
    public:
        // 设置模式
        void SetMode(RegionMode mode) { mode_ = mode; }

        void SetGridSize(uint32_t size) { grid_size_ = size; }

   

        // 输入：GridMap，输出：每个格子的区域 ID（0 表示不可行走）
        std::vector<uint32_t> MarkRegions(const std::shared_ptr<A_Star::GridMap> map);

        // 获取区域数量
        uint32_t GetRegionCount() const { return region_count_; }

    private:
        // 基于连通性的区域标记（原始算法）
        /*
            1. 遍历所有格子，只处理可行走格子

            2. 检查当前格子的上方和左方邻居：

            2.1. 如果上方和左方都不可行走 → 新区域

            2.2. 如果只有上方可行走 → 继承上方的区域 ID

            2.3. 如果只有左方可行走 → 继承左方的区域 ID

            2.4. 如果上方和左方都可行走但区域 ID 不同 → 合并两个区域

        3. 第二次遍历，统一区域 ID（路径压缩）
        */
        std::vector<uint32_t> MarkRegionsConnectivity(const std::shared_ptr<A_Star::GridMap> map);

        // 基于网格划分的区域标记（新增）
        std::vector<uint32_t> MarkRegionsGrid(const std::shared_ptr<A_Star::GridMap> map);


        // 并查集辅助
        uint32_t FindRoot(uint32_t x, std::vector<uint32_t>& parent);

        void MergeRegionId(uint32_t b, uint32_t a, std::vector<uint32_t>& parent);

    private:
        uint32_t region_count_ = 0;

        RegionMode mode_ = RegionMode::Connectivity;

        uint32_t grid_size_ = 2; // 网格模式下每个区域的大小（格子数）
    };
}