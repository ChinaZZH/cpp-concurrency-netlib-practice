#pragma once
#include "../../Common/AABB.h"
#include "Partition.h"
#include "LoadThresholds.h"
#include "MigrationDecision.h"
#include <unordered_map>
#include <unordered_set>
#include <vector>

class PartitionManager
{
public:
    // --- 初始化 ---
    void Init(const AABB& world_bounds, int grid_size);

    // --- 查询 ---
    std::shared_ptr<Partition> GetPartition(uint32_t partition_id);

    const std::shared_ptr<Partition> GetPartition(uint32_t partition_id) const;

    std::shared_ptr<Partition> GetPartitionByPos(uint32_t x, uint32_t y);

    std::vector<std::shared_ptr<Partition>> GetAllPartitions();

    // --- 管理 ---
    uint32_t CreatePartition(const AABB& bounds, const std::string& work_server_address);

    bool RemovePartition(uint32_t partition_id);

    bool UpdatePartitionState(uint32_t partition_id, PartitionState new_state);

    // --- 负载上报 ---
    bool UpdateLoad(uint32_t partition_id, uint32_t player_count, uint32_t entity_count);

    // --- AOI 访问 ---
    IAOIManager* GetPartitionAOI(uint32_t partition_id);

// --- 负载监测 与 迁移决策 ---
public:
    // --- 负载监测
    void RefreshAllLoads();

    void RefreshLoad(uint32_t partition_id);

    std::vector<uint32_t> GetOverloadedPartitions() const; // 获取超载分区列表

    std::vector<uint32_t> GetUnderloadedPartitions() const; 


    // --- 迁移决策
    std::vector<MigrationDecision> EvaluteAllPartitions();

    MigrationDecision EvaluatePartition(uint32_t partition_id);

    // 设置阈值
    void SetThresholds(const LoadThresholds& thresholds);
    const LoadThresholds& GetThresholds() const { return thresholds_; }
 
    

    // --- 节点管理
    void RegisterWorkServer(uint32_t work_server_id, const std::string& work_server_address);

    void UnregisterWokServer(uint32_t work_server_id);

private:
    // 内部辅助：创建分区时自动创建对应的 AOI 实例
    std::unique_ptr<IAOIManager> CreateAOIForPartition(const AABB& bounds);

    //////////////  工作服务器节点
    uint64_t GetCurrentTimeMs() const;

    uint32_t FindLeastLoadedWorkServer() const;

private:
    std::unordered_map<uint32_t, std::shared_ptr<Partition>> partitions_;
    
    AABB world_bounds_;
    
    int grid_size_;
    
    uint32_t next_partition_id_ = 1;

    LoadThresholds thresholds_;

    //////////////  工作服务器节点
    std::unordered_map<uint32_t, std::string> work_servers_list_;
    
    uint32_t next_work_server_id_ = 1;
};