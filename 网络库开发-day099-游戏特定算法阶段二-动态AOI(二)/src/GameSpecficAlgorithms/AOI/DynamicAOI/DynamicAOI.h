#pragma once


#include "IDynamicAOI.h"
#include "AABB.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cstdint>

struct RegionStats
{
    uint32_t region_id;
    uint32_t player_count;          //  当前区域玩家数
    int   area_size;                //  区域面积（用于计算密度）
    uint32_t last_split_frame;      //  上次分裂帧号（防抖动）
    uint32_t last_merge_frame;      //  上次合并帧号（防抖动）
};


struct RegionNode
{
    RegionStats stats;          // RegionStats
    AABB bounds;                // 区域边界
    uint32_t parent_id = 0;     // 父区域
    std::vector<uint32_t> children; // 子区域列表
    
    std::unordered_set<uint32_t> entity_id_list;
};



class DynamicAOI : public IDynamicAOI
{
public:
    DynamicAOI(int grid_size = 100);
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

   
public:
    std::vector<int> Query(int queryX, int queryY, int queryLength);

private:
    // ----- 核心访问接口（返回指针） -----
    RegionNode* GetRegion(uint32_t region_id);
    const RegionNode* GetRegion(uint32_t region_id) const;

    // ----- 内部核心方法 -----
    void SplitRegion(uint32_t region_id);
    void MergeRegion(uint32_t region_id);
    void CheckAndAdjust(uint32_t region_id);

    // ----- 辅助查找 -----
    uint32_t FindLeafRegion(uint32_t region_id, int x, int y) const;
    

    // ----- 辅助管理 -----
    // 分配新区域 ID
    uint32_t AllocateRegionId();
    void MoveEntityToRegion(uint32_t entity_id, uint32_t new_region_id, int new_x, int new_y);
     // ----- 实体迁移（分裂/合并时使用） -----
    void MigrateEntitiesFromRegion(uint32_t from_region_id, uint32_t to_region_id);

    // ----- 帧号管理 -----
    // 获取当前帧号（由外部注入或使用计数器）
    uint32_t GetCurrentFrame() const { return current_frame_; }
    void Tick()  { current_frame_ += 1; }
   

    RegionState CalcuRegionState(const RegionNode& node) const;

    std::vector<int> FindPosInAABB(uint32_t region_id, const AABB& neighborAABB) const;

private:
    // 整合后的单一存储：region_id -> RegionNode
    std::unordered_map<uint32_t, RegionNode>   regions_;

    // 实体位置映射：用于更新密度统计
    struct EntityRegionInfo
    {
        uint32_t region_id;
        std::pair<int, int> pos;
    };

    std::unordered_map<uint32_t, EntityRegionInfo> entity_to_region_;  // entity_id -> region_id


    // 阈值配置
    float split_threshold_ = 10.0f;         // 密度 > 10 玩家/单位面积 → 分裂
    float merge_threshold_ = 3.0f;          // 密度 < 3 玩家/单位面积 → 合并

    // 防抖动配置
    uint32_t split_cooldown_frames_  =   10;        // 分裂后至少等待10帧再次分裂
    uint32_t merge_cooldown_frames_  =   10;        // 合并后至少等待10帧再次分裂

    // 帧号计数器
    uint32_t current_frame_ = 0;

    // 区域 ID 分配器
    uint32_t next_region_id_ = 1;

    // 根区域 ID（整个地图范围）
    uint32_t root_region_id_ = 0;

    static constexpr int MAP_SIZE_ = 1024; // 必须为2的幂次方

    int grid_size_ = 100;
};