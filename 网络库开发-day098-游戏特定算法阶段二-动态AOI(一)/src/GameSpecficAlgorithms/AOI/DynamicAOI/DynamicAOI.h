#pragma once


#include "IDynamicAOI.h"
#include <unordered_map>

struct RegionStats
{
    uint32_t region_id;
    uint32_t player_count;          //  当前区域玩家数
    float area_size;                //  区域面积（用于计算密度）
    uint32_t last_split_frame;      //  上次分裂帧号（防抖动）
    uint32_t last_merge_frame;      //  上次合并帧号（防抖动）
};


class DynamicAOI : public IDynamicAOI
{
private:
    // 区域网格：将地图划分为粗粒度网格，每个网格独立统计 
    std::unordered_map<uint32_t, RegionStats>   region_stats_;

    // 实体位置映射：用于更新密度统计
    std::unordered_map<uint32_t, uint32_t> entity_to_region_;  // entity_id -> region_id

    // 阈值配置
    float split_threshold_ = 10.0f;         // 密度 > 10 玩家/单位面积 → 分裂
    float merge_threshold_ = 3.0f;          // 密度 < 3 玩家/单位面积 → 合并

    // 防抖动配置
    uint32_t split_cooldown_frames_  =   10;        // 分裂后至少等待10帧再次分裂
    uint32_t merge_cooldown_frames_  =   10;        // 合并后至少等待10帧再次分裂
};