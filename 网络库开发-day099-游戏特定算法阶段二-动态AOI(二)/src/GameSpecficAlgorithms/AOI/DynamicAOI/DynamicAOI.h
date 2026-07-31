#pragma once


#include "IDynamicAOI.h"
#include "AABB.h"
#include <unordered_map>
#include <vector>
#include <cstdint>

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
public:
    DynamicAOI();
    virtual ~DynamicAOI() = default;

    // ----- IAOIManager 接口实现 -----
public:
    virtual bool AddEntity(int entityId, int x, int y) override;
    virtual bool RemoveEntity(int entityId) override;
    virtual bool MoveEntity(int entityId, int newX, int newY) override;
    virtual std::vector<int> GetNeighbors(int entityId, int radius = 1) const override;
    virtual EntityPositionResult GetEntityPosition(int entityId) const override;

    // ----- IDynamicAOI 接口实现 -----
public:
    //  获取指定区域的玩家密度 (玩家数/区域面积)
    virtual float GetDensity(uint32_t region_id) const override;

    // 获取当前所有区域的 分裂状态/合并状态 （用于调试/日志）
    virtual std::vector<RegionInfo> GetRegionInfos() const override;

    // 手动触发一次区域重划分(用于测试或者主动调优)
    virtual void Rebalance() override;

    // 设置 分裂/合并 阈值(运行时调整)
    virtual void SetSplitThreshold(float threshold) override    { split_threshold_ = threshold; }
    virtual void SetMergeThreshold(float threshold) override    { merge_threshold_ = threshold; }

   
private:
    // ----- 内部核心方法 -----
    void SplitRegion(uint32_t region_id);
    void MergeRegion(uint32_t region_id);
    void CheckAndAdjust(uint32_t region_id);

    // 获取当前帧号（由外部注入或使用计数器）
    uint32_t GetCurrentFrame() const { return current_frame_; }
    void Tick()  { current_frame_ += 1; }

    // 分配新区域 ID
    uint32_t AllocateRegionId();

    // 获取实体所在区域
    uint32_t GetEntityRegion(uint32_t entity_id) const;

    // 区域边界管理
    AABB GetRegionBounds(uint32_t region_id) const;
    void SetRegionBounds(uint32_t region_id, const AABB& bounds);
    void RemoveRegionBounds(uint32_t region_id);

    // 层级管理
    uint32_t GetRegionParent(uint32_t region_id) const;
    void SetRegionParent(uint32_t region_id, uint32_t parent_id);
    void RemoveRegionParent(uint32_t region_id);

    std::vector<uint32_t> GetRegionChildren(uint32_t region_id) const;
    void AddRegionChild(uint32_t parent_id, uint32_t child_id);
    void RemoveRegionChild(uint32_t parent_id, uint32_t child_id);
    void ClearRegionChildren(uint32_t region_id);

    // 实体迁移
    void MOveEntityToRegion(uint32_t entity_id, uint32_t new_region_id);

    // 计算密度
    float CalcDensity(uint32_t region_id);

private:
    // 区域统计 区域网格：将地图划分为粗粒度网格，每个网格独立统计 
    std::unordered_map<uint32_t, RegionStats>   region_stats_;

    // 实体位置映射：用于更新密度统计
    std::unordered_map<uint32_t, uint32_t> entity_to_region_;  // entity_id -> region_id

    // 区域边界
    std::unordered_map<uint32_t, AABB> region_bounds_;

    // 层级结构：父区域
    std::unordered_map<uint32_t, uint32_t> region_parent_;

    // 层级结构：子区域列表
    std::unordered_map<uint32_t, std::vector<uint32_t>> region_children_;

    // 阈值配置
    float split_threshold_ = 10.0f;         // 密度 > 10 玩家/单位面积 → 分裂
    float merge_threshold_ = 3.0f;          // 密度 < 3 玩家/单位面积 → 合并

    // 防抖动配置
    uint32_t split_cooldown_frames_  =   10;        // 分裂后至少等待10帧再次分裂
    uint32_t merge_cooldown_frames_  =   10;        // 合并后至少等待10帧再次分裂

    // 帧号计数器
    uint32_t current_frame_ = 0;

    // 区域 ID 分配器
    uint32_t next_region_id = 1;

    // 根区域 ID（整个地图范围）
    uint32_t root_region_id_ = 0;
};