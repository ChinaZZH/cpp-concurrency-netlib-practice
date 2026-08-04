#pragma once
#include "../../Common/AABB.h"
#include "Partition.h"
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
    uint32_t CreatePartition(const AABB& bounds, const std::string& node_address);

    bool RemovePartition(uint32_t partition_id);

    bool UpdatePartitionState(uint32_t partition_id, PartitionState new_state);

    // --- 负载上报 ---
    bool UpdateLoad(uint32_t partition_id, uint32_t player_count, uint32_t entity_count);

    // --- AOI 访问 ---
    IAOIManager* GetPartitionAOI(uint32_t partition_id);

private:
    // 内部辅助：创建分区时自动创建对应的 AOI 实例
    std::unique_ptr<IAOIManager> CreateAOIForPartition(const AABB& bounds);

private:
    std::unordered_map<uint32_t, std::shared_ptr<Partition>> partitions_;
    
    AABB world_bounds_;
    
    int grid_size_;
    
    uint32_t next_partition_id_ = 1;
};