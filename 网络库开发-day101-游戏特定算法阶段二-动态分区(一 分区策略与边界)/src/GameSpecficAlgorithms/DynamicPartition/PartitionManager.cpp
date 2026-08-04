#include "PartitionManager.h"
#include "../AOI/GridAOI.h"
#include <cmath>
#include <iostream>
#include <chrono>

/*
std::unordered_map<uint32_t, Partition> partitions_;
    
    AABB world_bounds_;
    
    uint32_t grid_size_;
    
    uint32_t next_partition_id_ = 1;
    */

 // --- 初始化 ---
void PartitionManager::Init(const AABB& world_bounds, int grid_size)
{
    if(grid_size <= 0)
    {
        throw std::runtime_error("PartitionManager::Init grid_size <= 0");
    }

    world_bounds_ = world_bounds;
    grid_size_ = grid_size;
    partitions_.clear();
    next_partition_id_ = 1;

    // 根据gridSize进行分区
    int width_value = world_bounds.GetWidth();
    int col_count = width_value / grid_size + ((width_value % grid_size) != 0? 1 : 0);

    int height_value = world_bounds.GetHeight();
    int row_count = height_value / grid_size + ((height_value % grid_size) != 0? 1 : 0);
     
    for(uint32_t row = 0; row < row_count; ++row)
    {
        for(uint32_t col = 0; col < col_count; ++col)
        {
            int minX = world_bounds_.min_x + col * grid_size;
            int minY = world_bounds_.min_x + col * grid_size;
            int maxX = std::min(minX + grid_size, world_bounds_.max_x);
            int maxY = std::min(minY + grid_size, world_bounds_.max_y);

            AABB bounds{minX, minY, maxX, maxY};
            CreatePartition(bounds, "localhost");            
        }
    }

    std::cout << "[PartitionManager] Initialized with " << partitions_.size() << " partitions" << std::endl;
}

// --- 查询 ---
std::shared_ptr<Partition> PartitionManager::GetPartition(uint32_t partition_id)
{
    auto itr = partitions_.find(partition_id);
    if(itr == partitions_.end())
    {
        return nullptr;
    }

    return (itr->second);
}

const std::shared_ptr<Partition> PartitionManager::GetPartition(uint32_t partition_id) const
{
    auto itr = partitions_.find(partition_id);
    if(itr == partitions_.end())
    {
        return nullptr;
    }

    return (itr->second);
}

std::shared_ptr<Partition> PartitionManager::GetPartitionByPos(uint32_t x, uint32_t y)
{
    if(false == world_bounds_.Contains(x, y))
    {
        return nullptr;
    }

    for(auto& [id, ptr_partition] : partitions_)
    {
        if(ptr_partition->bounds.Contains(x, y))
        {
            return ptr_partition;
        }
    }

    return nullptr;
}

std::vector<std::shared_ptr<Partition>> PartitionManager::GetAllPartitions()
{
    std::vector<std::shared_ptr<Partition>> vecPartition;
    vecPartition.reserve(partitions_.size());
    for(auto& [id, ptr_partition] : partitions_)
    {
        vecPartition.emplace_back(ptr_partition);
    }

    return vecPartition;
}

// --- 管理 ---
uint32_t PartitionManager::CreatePartition(const AABB& bounds, const std::string& node_address)
{
    uint32_t new_partition_id = next_partition_id_;
    {
        next_partition_id_ += 1;
    }

    std::shared_ptr<Partition> new_partition_ptr = std::make_shared<Partition>();
    {
        new_partition_ptr->partition_id = new_partition_id;
        new_partition_ptr->bounds = bounds;                   // 分区边界（矩形）
        new_partition_ptr->node_address = node_address;      // 所在服务节点地址
        new_partition_ptr->state = PartitionState::Active;

        new_partition_ptr->player_count = 0;          // 当前玩家数（定期更新）
        new_partition_ptr->entity_count = 0;          // 当前实体数（定期更新）
        new_partition_ptr->last_update_time = 0;      // 上次负载更新时间
        new_partition_ptr->aoi = CreateAOIForPartition(bounds); // 直接持有该分区的 AOI 实例
    }

    partitions_[new_partition_id] = new_partition_ptr;
    return new_partition_id;
}

bool PartitionManager::RemovePartition(uint32_t partition_id)
{
    auto itr = partitions_.find(partition_id);
    if(itr == partitions_.end())
    {
        return false;
    }

    partitions_.erase(itr);
    return true;
}

bool PartitionManager::UpdatePartitionState(uint32_t partition_id, PartitionState new_state)
{
    auto itr = partitions_.find(partition_id);
    if(itr == partitions_.end())
    {
        return false;
    }

    std::shared_ptr<Partition>& partition = (itr->second);
    partition->state = new_state;
    return true;
}

// --- 负载上报 ---
bool PartitionManager::UpdateLoad(uint32_t partition_id, uint32_t player_count, uint32_t entity_count)
{
    auto itr = partitions_.find(partition_id);
    if(itr == partitions_.end())
    {
        return false;
    }

    std::shared_ptr<Partition>& partition = (itr->second);
    partition->player_count = player_count;
    partition->entity_count = entity_count;

    auto now = std::chrono::system_clock::now();
    partition->last_update_time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return true;
}

// --- AOI 访问 ---
IAOIManager* PartitionManager::GetPartitionAOI(uint32_t partition_id)
{
    auto itr = partitions_.find(partition_id);
    if(itr == partitions_.end())
    {
        return nullptr;
    }

    std::shared_ptr<Partition>& partition = (itr->second);
    return partition->aoi.get();
}


// 内部辅助：创建分区时自动创建对应的 AOI 实例
std::unique_ptr<IAOIManager> PartitionManager::CreateAOIForPartition(const AABB& bounds)
{
    // 默认使用 GridAOI（九宫格），因为在前面已经验证过它在大多数场景下表现最稳
    // 后续可以扩展到根据分区大小或负载动态选择 AOI 类型
    int minValue = std::min(bounds.GetWidth(), bounds.GetHeight());
    int gridSize = minValue / 10;
    return std::make_unique<GridAOI>(gridSize);
}