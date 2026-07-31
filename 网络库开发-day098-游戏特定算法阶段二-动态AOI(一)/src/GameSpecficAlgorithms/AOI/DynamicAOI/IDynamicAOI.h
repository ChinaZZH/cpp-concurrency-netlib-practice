#pragma once

#include "../IAOIManager.h"
#include <vector>

enum class RegionState
{
    Stable,         // 稳定状态（密度在阈值范围内）
    Splitting,      // 正在分裂中
    Merging,        // 正在合并中
    Empty,          // 空区域（无实体，待回收）
};


struct RegionInfo
{
    uint32_t region_id;         // 区域唯一标识
    uint32_t player_count;      // 区域内玩家数量
    float area_size;            // 区域面积（单位：地图单位²）
    //float density;            // 玩家密度 = player_count / area_size
    RegionState state;          // 当前状态
    std::vecotr<uint32_t> children;     // 子区域 ID 列表（如果有）
    uint32_t parent_id;         // 父区域 ID（0 表示根区域）
};

class IDynamicAOI: public IAOIManager
{
public:
    virtual ~IDynamicAOI() = default;

    //  ---  动态管理接口 ---  

    //  获取指定区域的玩家密度 (玩家数/区域面积)
    virtual float GetDensity(uint32_t region_id) const = 0;

    // 获取当前所有区域的 分裂状态/合并状态 （用于调试/日志）
    virtual std::vector<RegionInfo> GetRegionInfos() const = 0;

    // 手动触发一次区域重划分(用于测试或者主动调优)
    virtual void Rebalance() = 0;

    // 设置 分裂/合并 阈值(运行时调整)
    virtual void SetSplitThreshold(float threshold) = 0;
    virtual void SetMergeThreshold(float threshold) = 0;
};