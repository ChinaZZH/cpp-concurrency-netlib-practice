
#pragma once

#include <cstdint>
#include <string>

enum class MigrationReason
{
    None,               
    Overloaded,     // 玩家太多
    Underloaded,    // 玩家太少（考虑合并）
    NodeShutdown,   // 节点即将下线
    Manual,         // 手动触发
};

enum class MigrationDirection
{
    None,
    Split,          // 分裂：一个分区拆成多个
    Merge,          // 合并：多个分区合并为一个
    Relocate,       // 搬迁：整个分区迁移到另一个节点
};

struct MigrationDecision
{
    bool should_migrate = false;
    uint32_t source_partition_id   = 0;
    uint32_t target_partition_id   = 0;        // 目标分区（合并时使用）
    uint32_t target_work_server_id = 0;        // 目标的计算机节点

    MigrationReason reason = MigrationReason::None;
    MigrationDirection direction = MigrationDirection::None;
    std::string reason_desc;            // 可读描述（调试用）
};