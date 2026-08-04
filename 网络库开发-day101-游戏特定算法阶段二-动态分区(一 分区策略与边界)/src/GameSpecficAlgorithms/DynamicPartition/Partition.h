#pragma once
#include <string>
#include <memory>
#include "../../Common/AABB.h"
#include "../AOI/IAOIManager.h"

enum class PartitionState
{
    Active,         // 正常运行
    Locking,        // 锁定中，准备迁移（不接受新连接）
    Migrating,      // 迁移中（数据正在传输）
    Inactive,       // 已停用（可以安全释放）
};


struct Partition
{
    uint32_t partition_id;
    AABB bounds;                    // 分区边界（矩形）
    std::string node_address;       // 所在服务节点地址
    PartitionState state;
    uint32_t player_count;          // 当前玩家数（定期更新）
    uint32_t entity_count;          // 当前实体数（定期更新）
    uint64_t last_update_time;      // 上次负载更新时间

    // 直接持有该分区的 AOI 实例
    std::unique_ptr<IAOIManager> aoi;
};