markdown

# 动态分区模块设计与实现

## 概述

动态分区是无缝大地图跨服迁移的核心基础设施。它将游戏世界划分为多个独立分区（Partition），每个分区承载一定范围内的玩家和实体，并可动态迁移到不同的服务器节点，从而实现：

- **弹性伸缩**：根据负载动态调整分区分布

- **故障转移**：节点下线时自动迁移分区到其他节点

- **负载均衡**：将超载分区的玩家分散到轻载分区

- **无缝体验**：玩家跨越分区边界时，服务端自动迁移状态，客户端无需断线重连

---

## 一、核心概念

### 1.1 分区（Partition）

分区是地图上的一块矩形区域，包含该区域内的所有玩家和实体。每个分区拥有独立的AOI实例，负责管理其内部的实体空间索引。

**分区状态**：
- `Active`：正常运行，接受新连接
- `Locking`：锁定中，准备迁移，不接受新操作
- `Migrating`：迁移中，数据正在传输
- `Inactive`：已停用，可安全释放

### 1.2 工作服务器（Work Server）

工作服务器是实际承载分区运行的物理/虚拟节点。一个节点可同时承载多个分区，一个分区只能归属于一个节点。

### 1.3 迁移（Migration）

迁移是将分区数据从一个节点传输到另一个节点的过程。迁移分为三种类型：

| 类型 | 描述 | 触发条件 |
| :--- | :--- | :--- |
| **Relocate** | 整个分区跨节点搬迁 | 节点下线、负载均衡 |
| **Split** | 一个分区拆分为两个 | 分区超载，玩家密度过高 |
| **Merge** | 两个分区合并为一个 | 分区轻载，玩家密度过低 |

---

## 二、架构设计

### 2.1 模块划分

┌─────────────────────────────────────────────────────────┐
│ 定时器驱动层 │
│ (每 5 秒触发负载检测) │
└─────────────────────┬───────────────────────────────────┘
▼
┌─────────────────────────────────────────────────────────┐
│ PartitionManager（决策层） │
│ - 分区创建/查询/删除 │
│ - 负载统计与刷新 │
│ - 分裂/合并/搬迁决策生成 │
└─────────────────────┬───────────────────────────────────┘
▼
┌─────────────────────────────────────────────────────────┐
│ MigrationManager（执行层） │
│ - 迁移状态机管理 │
│ - 数据序列化/反序列化 │
│ - 跨节点网络传输 │
│ - 源节点资源释放 │
└─────────────────────────────────────────────────────────┘

### 2.2 迁移状态机

Idle → Locking → Serializing → Transferring → Activating → Done
↑ │
└────────── Rollback ──────────────────┘

| 状态 | 含义 |
| :--- | :--- |
| Idle | 空闲，可发起迁移 |
| Locking | 锁定分区，拒绝新操作 |
| Serializing | 序列化玩家数据 |
| Transferring | 网络传输中 |
| Activating | 目标节点恢复中 |
| Done | 迁移完成，可清理源节点 |
| Rollback | 迁移失败，回滚 |

### 2.3 与AOI的关系

动态分区不关心AOI内部是网格法、四叉树还是动态AOI，只通过 `IAOIManager` 接口与AOI交互：

```cpp
// 分区持有自己的AOI实例
struct Partition {
    std::unique_ptr<IAOIManager> aoi;
};

// 通过接口获取实体列表
auto entities = partition->aoi->GetAllEntities();
```

## 三、核心数据结构

### 3.1 Partition
```cpp
struct Partition {
    uint32_t partition_id;
    AABB bounds;                         // 区域边界
    std::string node_address;            // 所在节点地址
    PartitionState state;
    uint32_t player_count;
    uint32_t entity_count;
    uint64_t last_update_time;
    std::unique_ptr<IAOIManager> aoi;    // 分区专属AOI
};
```

### 3.2 LoadThresholds
```cpp
struct LoadThresholds {
    uint32_t max_players_per_partition = 50;   // 超载阈值
    uint32_t max_entities_per_partition = 200;
    uint32_t min_players_for_merge = 5;        // 轻载阈值
    float check_interval_seconds = 5.0f;       // 检查间隔
    uint32_t consecutive_overloads = 3;        // 连续超载次数
};
```

### 3.3 MigrationDecision
```cpp
struct MigrationDecision {
    bool should_migrate;
    uint32_t source_partition_id;
    uint32_t target_partition_id;   // 合并时使用
    uint32_t target_node_id;        // 跨节点迁移时使用
    MigrationReason reason;
    MigrationDirection direction;   // Split / Merge / Relocate
    std::string reason_desc;
};
```

### 3.4 迁移协议（Protobuf）
```protobuf
message PlayerState {
    uint32 player_id = 1;
    float x = 2;
    float y = 3;
    float hp = 4;
}

message MigrationData {
    uint32 partition_id = 1;
    uint32 source_node_id = 2;
    uint32 target_node_id = 3;
    uint64 snapshot_time = 4;
    repeated PlayerState players = 5;
    uint32 total_players = 6;
}


message MigrationAck {
    uint32 partition_id = 1;
    bool success = 2;
    string error_message = 3;
}
```
## 四、关键流程

### 4.1 分裂流程（Split）

-- 1. 创建新分区，分配新的AOI实例 四分法

-- 2. 将源分区的实体按照分区对应的坐标位置进行转移到新的AOI

-- 3. 更新新分区的负载统计

-- 4. 移除源分区

### 4.2 合并流程（Merge）

-- 1.  从目标分区获取所有实体

-- 2. 同时将目标分区的区域拓展为两者合并的区域

-- 3. 将所有实体迁移到源分区

-- 4.  删除目标分区

-- 5.  更新源分区的负载统计

### 4.3 跨节点迁移流程（Relocate）

-- 1.  锁定：源分区状态设为 Locking

-- 2.  序列化：通过AOI获取所有实体，打包为 MigrationData

-- 3.  传输：通过 TcpServer 发送到目标节点

-- 4.  恢复：目标节点反序列化并恢复实体到新分区

-- 5.  确认：目标节点回复 MigrationAck

-- 6.  清理：源节点释放资源，分区状态变为 Inactive


## 五、设计要点总结

|设计选择	| 说明|
| :--- | :--- |
|职责分离	| PartitionManager 负责决策，MigrationManager 负责执行|
|定时器驱动	| 避免频繁检查造成性能开销|
|AOI 接口依赖	| 不依赖具体AOI实现，只通过 IAOIManager 交互|
|状态机驱动	| 迁移过程有明确状态流转，可回滚|
|异步传输	| 数|据迁移通过消息队列解耦，不阻塞主循环|
|增量恢复	| 目标节点保留已有实体，仅恢复迁移过来的数据|

## 六、后续扩展方向

-- 1.  动态分区大小调整：根据实体密度自动调整分区边界

-- 2.  节点健康检查：自动检测节点故障并触发迁移

-- 3.  迁移可视化：在管理后台展示分区分布和负载热力图

-- 4.  跨区玩家同步：分区边界附近的玩家可互相感知

-- 5.  迁移优先级：根据分区重要性设定不同迁移优先级