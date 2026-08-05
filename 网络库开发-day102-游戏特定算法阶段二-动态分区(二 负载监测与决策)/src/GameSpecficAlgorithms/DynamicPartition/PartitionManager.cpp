#include "PartitionManager.h"
#include "../AOI/GridAOI.h"
#include <cmath>
#include <iostream>
#include <sstream>
#include <chrono>
#include <algorithm>


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
uint32_t PartitionManager::CreatePartition(const AABB& bounds, const std::string& work_server_address)
{
    uint32_t new_partition_id = next_partition_id_;
    {
        next_partition_id_ += 1;
    }

    std::shared_ptr<Partition> new_partition_ptr = std::make_shared<Partition>();
    {
        new_partition_ptr->partition_id = new_partition_id;
        new_partition_ptr->bounds = bounds;                   // 分区边界（矩形）
        new_partition_ptr->work_server_address = work_server_address;      // 所在服务节点地址
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
    partition->last_update_time = this->GetCurrentTimeMs();
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


/*
std::unordered_map<uint32_t, Partition> partitions_;
    
    AABB world_bounds_;
    
    uint32_t grid_size_;
    
    uint32_t next_partition_id_ = 1;

    LoadThresholds thresholds_;

    //////////////  工作服务器节点
    std::unordered_map<uint32_t, std::string> work_servers_list_;
    
    uint32_t next_work_server_id_ = 1;
    */

//---------------------------------------------------------------------------------------------
// --- ----------------------------------负载监测 与 迁移决策 -----------------------------------
// --------------------------------------负载监测 与 迁移决策 -----------------------------------
// ------------------------------------- 负载监测 与 迁移决策 -----------------------------------
//---------------------------------------------------------------------------------------------

// ------------------------------------- 负载监测
void PartitionManager::RefreshAllLoads()
{
    for(const auto& [id, partition] : partitions_)
    {
        this->RefreshLoad(id);
    }
}

void PartitionManager::RefreshLoad(uint32_t partition_id)
{
    // 1.判断是否能够找到这个分区
    auto itr = partitions_.find(partition_id);
    if(itr == partitions_.end())
    {
        return ;
    }

    // 2.判断分区的状态，如果不是正常运行，则不能做迁移的决定。
    std::shared_ptr<Partition>& ptr_partition = (itr->second);
    if(!(ptr_partition->aoi))
    {
        return;
    }

    auto entities = (ptr_partition->aoi)->GetAllEntities();
    ptr_partition->player_count = entities.size();
    ptr_partition->entity_count = (ptr_partition->player_count);
    ptr_partition->last_update_time = this->GetCurrentTimeMs();
}

// 获取超载分区列表
std::vector<uint32_t> PartitionManager::GetOverloadedPartitions() const
{
    std::vector<uint32_t> result;
    for(const auto& [id, partition] : partitions_)
    {
        if((PartitionState::Active == partition->state) && (partition->player_count > thresholds_.max_players_per_partition))
        {
            result.push_back(id);
        }
    }

    return result;
}


std::vector<uint32_t> PartitionManager::GetUnderloadedPartitions() const
{
    std::vector<uint32_t> result;
    for(const auto& [id, partition] : partitions_)
    {
        if((PartitionState::Active == partition->state) && (partition->player_count < thresholds_.min_players_for_merge))
        {
            result.push_back(id);
        }
    }

    return result;
}

// --- 迁移决策
std::vector<MigrationDecision> PartitionManager::EvaluteAllPartitions()
{
    std::vector<MigrationDecision> result;
    for(const auto& [id, partition] : partitions_)
    {
        MigrationDecision decision = this->EvaluatePartition(id);
        if(decision.should_migrate)
        {
            result.push_back(decision);
        }
    }

    return result;
}


MigrationDecision PartitionManager::EvaluatePartition(uint32_t partition_id)
{
    MigrationDecision decision;
    decision.should_migrate = false;

    // 1.判断是否能够找到这个分区
    auto itr = partitions_.find(partition_id);
    if(itr == partitions_.end())
    {
        return decision;
    }

    // 2.判断分区的状态，如果不是正常运行，则不能做迁移的决定。
    std::shared_ptr<Partition>& ptr_partition = (itr->second);
    {
        if(PartitionState::Active != (ptr_partition->state))
        {
            return decision;
        }
    }

    decision.source_partition_id = partition_id;
    // 检查是否超载
    {

        if((ptr_partition->player_count) > thresholds_.max_players_per_partition)
        {
            decision.should_migrate = true;
            // decision.target_partition_id   
            decision.target_work_server_id = FindLeastLoadedWorkServer();
            decision.reason = MigrationReason::Overloaded;
            decision.direction = MigrationDirection::Split;

            std::stringstream ss;
            ss << "Overloaded: " << (ptr_partition->player_count) << " > " << (thresholds_.max_players_per_partition);
            decision.reason_desc = ss.str();
            return decision;
        }
    }

    // 检查轻载（可合并）
    {
        if((ptr_partition->player_count) < thresholds_.min_players_for_merge)
        {
            // 找到相邻的结点，然后合并
            const AABB& srcAABB = ptr_partition->bounds;
            auto itr_merge = std::find_if(partitions_.begin(), partitions_.end(), [this, partition_id, &srcAABB](const auto& kv){
                return  (partition_id != kv.first) &&
                        (PartitionState::Active == kv.second->state) &&
                        (kv.second->player_count < thresholds_.min_players_for_merge) &&
                        (kv.second->bounds.IsAdjacentTo(srcAABB));
            });

            if(itr_merge != partitions_.end())
            {
                decision.should_migrate = true;
                decision.target_partition_id = (itr_merge->first);  
                decision.target_work_server_id = FindLeastLoadedWorkServer();
                decision.reason = MigrationReason::Underloaded;
                decision.direction = MigrationDirection::Merge;

                std::stringstream ss;
                ss << partition_id << " Underloaded, Merge with " << (itr_merge->first);
                decision.reason_desc = ss.str();
                return decision;
            }
        }
        
    }

    decision.should_migrate = false;
    return decision;
}

// 设置阈值
void PartitionManager::SetThresholds(const LoadThresholds& thresholds)
{
    thresholds_ = thresholds;
}
 
// --- 节点管理
void PartitionManager::RegisterWorkServer(uint32_t work_server_id, const std::string& work_server_address)
{
    work_servers_list_[work_server_id] = work_server_address;
}

void PartitionManager::UnregisterWokServer(uint32_t work_server_id)
{
    work_servers_list_.erase(work_server_id);
}


//////////////  工作服务器节点
uint64_t PartitionManager::GetCurrentTimeMs() const
{
    auto now = std::chrono::system_clock::now();
    uint64_t current_time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
    return current_time;
}

uint32_t PartitionManager::FindLeastLoadedWorkServer() const
{
    // 简化实现：如果没有注册节点，返回 0（表示当前节点）
    if(work_servers_list_.empty())
    {
        return 0;
    }

    // 找到负载最低的节点
    // 由于目前没有节点负载统计，简单返回第一个
    return (work_servers_list_.begin()->first);
}