#pragma once

#include <unordered_map>
#include <unordered_set>
#include <functional>
#include <memory>
#include "PartitionManager.h"
#include "../../../build/proto_gen/migration.pb.h"

enum class MigrationState {
    Idle,
    Locking,
    Serializing,
    Transferring,
    Activating,
    Done,
    Rollback,
};

class MigrationManager
{
public:
    MigrationManager(std::shared_ptr<PartitionManager> partition_mgr);


// --- 核心接口 ---
public:
    // --- --------------------------------- ---
    // --- 核心接口 ---
    // --- --------------------------------- ---
    
    // 发起迁移
    bool StartMigration(uint32_t partition_id, uint32_t target_node_id);

    // 接受迁移数据(目标节点受到数据时调用)
    bool ReceiveMigrationData(const MigrationData& data);

    // 确认迁移完成(目标节点回复完成后调用)
    bool ConfirmMigration(uint32_t partition_id);

    // 取消/回滚 迁移
    bool RollbackMigration(uint32_t partition_id);

    // --- --------------------------------- ---
    // --- 状态查询 ---
    // --- --------------------------------- ---

    // 获取迁移状态
    MigrationState GetMigrationState(uint32_t partition_id) const;

    bool IsMigrating(uint32_t partition_id) const;

    // --- --------------------------------- ---
    // --- 回调设置(通知上层，如网络层) ---
    // --- --------------------------------- ---
    using OnDataReadyCallback = std::function<void(const MigrationData& data)>;
    void SetOnDataReadyCallback(OnDataReadyCallback cb) { on_data_ready_cb_ = std::move(cb); }

    using OnMigrationCompleteCallback = std::function<void(uint32_t partition_id, bool success)>;
    void SetOnMigrationCompleteCallback(OnMigrationCompleteCallback cb) { on_complete_cb_ = std::move(cb); }

private:
    // 状态转换
    void SetState(uint32_t partition_id, MigrationState new_state);

    // 序列化分区数据
    MigrationData SerializePartition(uint32_t partition_id);

    // 反序列化并回复分区数据
    bool DeserializeAndResetore(const MigrationData& data);

    // 清理源节点
    void CleanupSource(uint32_t partition_id);

private:
    std::shared_ptr<PartitionManager> partition_mgr_;

    std::unordered_map<uint32_t, MigrationState> migration_states_;

    OnDataReadyCallback on_data_ready_cb_;

    OnMigrationCompleteCallback on_complete_cb_;
};