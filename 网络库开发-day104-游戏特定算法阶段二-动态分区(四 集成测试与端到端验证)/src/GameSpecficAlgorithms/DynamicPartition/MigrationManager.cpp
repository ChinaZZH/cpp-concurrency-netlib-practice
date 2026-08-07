#include "MigrationManager.h"
#include <sstream>

MigrationManager::MigrationManager(std::shared_ptr<PartitionManager> partition_mgr)
:partition_mgr_(partition_mgr)
{

}

void MigrationManager::OnTimerTick()
{
    // 1. 刷新所有分区的负载数据
    partition_mgr_->RefreshAllLoads();

    // 2. 获取所有迁移决策
    auto decisions = partition_mgr_->EvaluteAllPartitions();

    for(const auto& decision : decisions) 
    {
        if (!decision.should_migrate) 
        {
            continue;
        }

        // 检查是否已经在迁移中，避免重复触发
        if(this->IsMigrating(decision.source_partition_id)) 
        {
            continue;
        }

        switch (decision.direction) {
            case MigrationDirection::Split:
                // 分裂：创建一个新分区，然后将部分玩家迁移过去
                // 这里需要先创建新分区，再执行迁移
                //HandleSplit(decision);
                break;

            case MigrationDirection::Merge:
                // 合并：将目标分区迁移到源分区，然后删除目标分区
                //HandleMerge(decision);
                break;

            case MigrationDirection::Relocate:
                // 跨节点迁移：整个分区搬走
                //this->StartMigration(decision.source_partition_id, decision.target_node_id);
                break;

            default:
                break;
        }
    }
}

// --- --------------------------------- ---------------------------------------------------------------------
// --- ----------------------核心接口 ---------------------------------------------------------------------
// --- --------------------------------- -------------------------------------------------------------------

// 发起迁移
bool MigrationManager::StartMigration(uint32_t partition_id, uint32_t target_node_id)
{
    // 1. 检查分区是否存在且为 Active 状态
    std::shared_ptr<Partition> partition = partition_mgr_->GetPartition(partition_id);
    
    {
        if(!partition)
        {
            std::cerr << "[Migration] Partition " << partition_id << " not found" << std::endl;
            return false;
        }

        if(PartitionState::Active != partition->state)
        {
            std::cerr << "[Migration] Partition " << partition_id << " is not Active" << std::endl;
            return false;
        }

        if(MigrationState::Idle != this->GetMigrationState(partition_id))
        {
            std::cerr << "[Migration] Partition " << partition_id << " is already migrating" << std::endl;
            return false;
        }
    }
    

    // 2. 锁定分区
    {
        SetState(partition_id, MigrationState::Locking);

        partition->state = PartitionState::Locking;
        
        std::cout << "[Migration] Partition " << partition_id << " locked" << std::endl;
    }
    

    // 3. 序列化数据
    MigrationData data;
    {
        SetState(partition_id, MigrationState::Serializing);

        auto now = std::chrono::system_clock::now();
        uint64_t current_time = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();

        data = this->SerializePartition(partition_id);
        data.set_source_node_id(0); // 当前节点
        data.set_target_node_id(target_node_id);
        data.set_snapshot_time(current_time);

        std::cout << "[Migration] Partition " << partition_id;
        std::cout << " serialized " << data.total_players() << " players" << std::endl;
    }
    

    // 4. 传输数据（通过回调交给上层）
    SetState(partition_id, MigrationState::Transferring);
    if(on_data_ready_cb_)
    {
        on_data_ready_cb_(data);
    }
    else
    {
        std::cerr << "[Migration] No data ready callback set!" << std::endl;
        //SetState(partition_id, MigrationState::Rollback);
        SetState(partition_id, MigrationState::Idle);
        partition->state = PartitionState::Active;
        return false;
    }



    return true;
}

// 接受迁移数据(目标节点受到数据时调用)
bool MigrationManager::ReceiveMigrationData(const MigrationData& data)
{
    uint32_t partition_id = data.partition_id();

    std::cout << "[Migration] Received migration data for partition " << partition_id;
    std::cout << " (" << data.total_players() << " players)" << std::endl;

    // 1. 在目标节点恢复数据
    bool migration_data = false;
    std::stringstream ss;
    do
    {
        SetState(partition_id, MigrationState::Activating);
        bool success = DeserializeAndResetore(data);
        if(!success) {
            SetState(partition_id, MigrationState::Idle); // 还原
            ss << "DeserializeAndResetore Fail";
            std::cerr << "[Migration] Failed to restore partition " << partition_id << std::endl;
            break;
        }

        
        // 2. 确认迁移完成
        SetState(partition_id, MigrationState::Done);
        std::cout << "[Migration] Partition " << partition_id << " restored successfully" << std::endl;
        migration_data = true;
        ss << "Successed";

        break;

    }while(0);

    
    if(on_ack_cb_)
    {
        on_ack_cb_(data.source_node_id(), partition_id, migration_data, ss.str());
    }

    return true;
}

// 确认迁移完成
bool MigrationManager::ConfirmMigration(uint32_t partition_id)
{
    bool result = false;
    std::stringstream ss;
    do
    {
        MigrationState state = GetMigrationState(partition_id);
        if(MigrationState::Transferring != state)
        {
            std::cerr << "[Migration] Partition " << partition_id << " not in Done state" << std::endl;
            ss << "MigrationState state error state:=" << static_cast<int>(state);
            break;
        }

        std::shared_ptr<Partition> ptr_partition = partition_mgr_->GetPartition(partition_id);
        if(!ptr_partition)
        {
            ss << "RollBack Failed partition nullptr";
            break;
        }

        // 清理分区
        CleanupSource(partition_id);
        ptr_partition->state = PartitionState::Inactive;
        ss << "Successed";
        result = true;
        break;
    }while(0);
    

    if(on_complete_cb_)
    {
        on_complete_cb_(partition_id, result, ss.str());
    }

   return true;
}

// 取消/回滚 迁移
bool MigrationManager::RollbackMigration(uint32_t partition_id)
{
    std::stringstream ss;
    do
    {
        std::shared_ptr<Partition> ptr_partition = partition_mgr_->GetPartition(partition_id);
        if(!ptr_partition)
        {
            ss << "RollBack Failed partition nullptr";
            break;
        }

        SetState(partition_id, MigrationState::Rollback);
        ptr_partition->state = PartitionState::Active;

        std::cout << "[Migration] Partition " << partition_id << " rolled back" << std::endl;
        ss << "RollBack Failed";
        break;

    }while(0);
    
    if(on_complete_cb_)
    {
        on_complete_cb_(partition_id, false, ss.str());
    }

    return true;
}

// --- ------------------------------------------------------------------------------------------------------
// --- ----------------------状态查询 ---------------------------------------------------------------------
// --- ----------------------------------------------------------------------------------------------------

// 获取迁移状态
MigrationState MigrationManager::GetMigrationState(uint32_t partition_id) const
{
    auto itr = migration_states_.find(partition_id);
    if(itr == migration_states_.end())
    {
        return MigrationState::Idle;
    }

    return (itr->second);
}



bool MigrationManager::IsMigrating(uint32_t partition_id) const
{
    MigrationState state = this->GetMigrationState(partition_id);
    return (MigrationState::Idle != state && MigrationState::Done != state);
}


// --- ------------------------------------------------------------------------------------------------------
// --- ----------------------内部辅助函数 ---------------------------------------------------------------------
// --- ----------------------------------------------------------------------------------------------------

// 状态转换
void MigrationManager::SetState(uint32_t partition_id, MigrationState new_state)
{
    migration_states_[partition_id] = new_state;
}

// 序列化分区数据
MigrationData MigrationManager::SerializePartition(uint32_t partition_id)
{
    MigrationData data;
    data.set_partition_id(partition_id);
    data.set_total_players(0);

    std::shared_ptr<Partition> partition = partition_mgr_->GetPartition(partition_id);
    if(!partition || !(partition->aoi))
    {
        return data;
    }

    auto vecEntities = std::move(partition->aoi->GetAllEntities());
    for(const auto& info : vecEntities)
    {
        MsgPlayerState* player_state = data.add_players();
        player_state->set_player_id(info.id);
        player_state->set_x(info.x);
        player_state->set_y(info.y);
        // TODO: 从 ServerPlayerManager 获取血量、等级等完整状态
    }

    data.set_total_players(vecEntities.size());
    return data;
}

// 反序列化并回复分区数据
bool MigrationManager::DeserializeAndResetore(const MigrationData& data)
{
    uint32_t partition_id = data.partition_id();

    // 检查分区是否已存在（如果目标节点是新节点，需要先创建分区）
    std::shared_ptr<Partition> ptr_partition = partition_mgr_->GetPartition(partition_id);
    if(!ptr_partition)
    {
         // 需要知道分区边界信息（从请求中传入）
        std::cerr << "[Migration] Partition " << partition_id << " not found on target node" << std::endl;
        return false;
    }

    // 确保 AOI 已初始化
    if(!ptr_partition->aoi) {
        std::cerr << "[Migration] Partition " << partition_id << " has no AOI instance" << std::endl;
        return false;
    }

    // 恢复所有玩家
    int restored = 0;
    for(const auto& player_state : data.players())
    {
        bool success_flag = (ptr_partition->aoi)->AddEntity(player_state.player_id(), player_state.x(), player_state.y());
        if(success_flag)
        {
            restored += 1;
        }
    }

    ptr_partition->player_count = (ptr_partition->aoi)->GetAllEntities().size();
    ptr_partition->entity_count = (ptr_partition->player_count);
    ptr_partition->state = PartitionState::Active;

    std::cout << "[Migration] Restored " << restored << " new entities to partition " << partition_id; 
    std::cout << ", total entities: " << (ptr_partition->player_count) << std::endl;
    return true;
}

// 清理源节点
void MigrationManager::CleanupSource(uint32_t partition_id)
{
    std::shared_ptr<Partition> ptr_partition = partition_mgr_->GetPartition(partition_id);
    if(!ptr_partition)
    {
        return;
    }

    int clean_entity_count = 0;
    if(ptr_partition->aoi) 
    {
       auto vecAllEntities = (ptr_partition->aoi)->GetAllEntities();
       clean_entity_count = vecAllEntities.size();

       for(const auto& entity : vecAllEntities)
       {
            ptr_partition->aoi->RemoveEntity(entity.id);
       }
    }

    std::cout << "[Migration] Source partition " << partition_id; 
    std::cout << " cleaned up (" << clean_entity_count << " entities removed)" << std::endl;   
}