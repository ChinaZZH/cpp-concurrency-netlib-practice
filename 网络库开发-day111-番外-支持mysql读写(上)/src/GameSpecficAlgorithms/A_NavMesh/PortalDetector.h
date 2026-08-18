#pragma once


#include <vector>
#include <unordered_map>
#include <cstdint>
#include <cmath>
#include <memory>
#include "NavMeshTypes.h"
#include "../A_Star/GridMap.h"

namespace NavMesh
{
    // 门户结构：表示两个区域之间的可通行连接
    struct Portal
    {
        uint32_t region_id_first;
        uint32_t region_id_second;
        Point2D center;             // 门户中心点（世界坐标）
        float width;                // 门户宽度（世界单位）
    };

    // 候选邻居检测 + 门户检测合一
    class PortalDetector
    {
    public:
        PortalDetector(const std::vector<RegionPolygon>& regions, 
            const std::vector<uint32_t>& region_ids, 
            const std::shared_ptr<A_Star::GridMap> map
        );

        // 检测所有门户
        void DetectPortals();

        // 获取结果
        const std::vector<Portal>& GetPortals() const { return portals_; }

         // 构建连接图（region_id → 相邻区域 ID 列表）
        std::unordered_map<uint32_t, std::vector<uint32_t>> BuildConnectionGraph() const;

    private:
        // 核心方法: 遍历网格边界， 找到所有候选邻居对， 并检测门户
        void DetectCandidatesAndPortals();    

        // 对某一对区域检测是否存在可通行缺口
        bool FindPortalBetween(uint32_t ra, uint32_t rb, Portal& out_portal) const;

        // 辅助：获取网格 (x,y) 的区域 ID（0 表示不可行走）
        uint32_t GetRegionId(int x, int y) const;

        // 辅助：计算两个区域共享边界的中点
        bool FindSharedEdgeMidpoint(uint32_t region_a, uint32_t region_b, Point2D& mid_point) const;

        // 辅助：在共享边界上检测可通行缺口
        bool FindPassableGap(uint32_t region_a, uint32_t region_b, Point2D& center, float& width) const;

    private:
        std::vector<RegionPolygon> regions_;

        std::vector<uint32_t> region_ids_;
        
        std::shared_ptr<A_Star::GridMap> map_;

        std::vector<Portal> portals_;

        // 缓存：区域 ID → 区域索引（加速查找）
        std::unordered_map<uint32_t, size_t> region_index_map_;
    };
}