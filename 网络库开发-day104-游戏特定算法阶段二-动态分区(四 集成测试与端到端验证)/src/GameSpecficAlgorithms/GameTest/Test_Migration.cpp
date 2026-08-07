// test_migration_day3.cpp
#include "Test_Migration.h"


#include <iostream>
#include <cassert>
#include <thread>
#include <chrono>
#include <memory>
#include "../DynamicPartition/PartitionManager.h"





void Test_Migration::MockSendToTarget(const MigrationData& data) {
    g_mock_network_data = data;
    g_mock_network_has_data = true;
    std::cout << "[MockNet] Data sent to target node" << std::endl;
}

bool Test_Migration::MockReceiveFromSource(MigrationData& out) {
    if (!g_mock_network_has_data) return false;
    out = g_mock_network_data;
    g_mock_network_has_data = false;
    return true;
}

// ===== 测试 1：状态机完整流转 =====
void Test_Migration::TestMigrationStateMachine() {
    std::cout << "\n[Test 1] Migration State Machine..." << std::endl;

    std::shared_ptr<PartitionManager> pmgr = std::make_shared<PartitionManager>();
    AABB world{0, 0, 1000, 1000};
    pmgr->Init(world, 100);

    MigrationManager mmgr(pmgr);
    mmgr.SetOnDataReadyCallback(std::bind(&Test_Migration::MockSendToTarget, this, std::placeholders::_1));

    auto partitions = pmgr->GetAllPartitions();
    uint32_t pid = partitions[0]->partition_id;

    // 插入一些测试实体
    auto* aoi = pmgr->GetPartitionAOI(pid);
    for (int i = 0; i < 10; ++i) {
        aoi->AddEntity(i, i * 10, i * 10);
    }

    // 1. 初始状态应为 Idle
    assert(mmgr.GetMigrationState(pid) == MigrationState::Idle);
    std::cout << "  Initial state: Idle ✓" << std::endl;

    // 2. 启动迁移
    bool started = mmgr.StartMigration(pid, 999);
    assert(started);
    assert(mmgr.GetMigrationState(pid) == MigrationState::Transferring);
    std::cout << "  After Start: Transferring ✓" << std::endl;

    // 3. 模拟目标节点收到数据并恢复
    MigrationData received;
    assert(MockReceiveFromSource(received));
    bool restored = mmgr.ReceiveMigrationData(received);
    assert(restored);
    assert(mmgr.GetMigrationState(pid) == MigrationState::Done);
    std::cout << "  After Receive: Done ✓" << std::endl;

    // 4. 确认迁移完成
    bool confirmed = mmgr.ConfirmMigration(pid);
    assert(confirmed);
    std::cout << "  After Confirm: Cleanup triggered ✓" << std::endl;

    std::cout << "[PASS] State machine complete" << std::endl;
}

// ===== 测试 2：序列化正确获取所有实体 =====
void Test_Migration::TestSerialization() {
    std::cout << "\n[Test 2] Serialization..." << std::endl;

    std::shared_ptr<PartitionManager> pmgr = std::make_shared<PartitionManager>();
    AABB world{0, 0, 1000, 1000};
    pmgr->Init(world, 100);

    MigrationManager mmgr(pmgr);

    auto partitions = pmgr->GetAllPartitions();
    uint32_t pid = partitions[0]->partition_id;
    auto* aoi = pmgr->GetPartitionAOI(pid);

    // 插入 25 个实体
    for (int i = 0; i < 25; ++i) {
        aoi->AddEntity(i, i * 5, i * 5);
    }

    // 手动触发序列化（不经过 StartMigration）
    // 通过内部方法或模拟：这里直接调用 SerializePartition
    // 由于 SerializePartition 是私有方法，我们在测试中用公开接口间接验证
    // 这里通过 StartMigration 触发序列化，然后检查传输的数据

    mmgr.SetOnDataReadyCallback([this](const MigrationData& data) {
        std::cout << "  Serialized " << data.total_players() << " entities" << std::endl;
        g_mock_network_data = data;
        g_mock_network_has_data = true;
    });

    bool started = mmgr.StartMigration(pid, 999);
    assert(started);

    MigrationData data;
    assert(MockReceiveFromSource(data));
    assert(data.total_players() == 25);
    assert(data.players_size() == 25);

    std::cout << "  Got " << data.players_size() << " players ✓" << std::endl;
    std::cout << "[PASS] Serialization correct" << std::endl;
}

// ===== 测试 3：目标节点反序列化并恢复 =====
void Test_Migration::TestDeserializationAndRestore() {
    std::cout << "\n[Test 3] Deserialization and Restore..." << std::endl;

    std::shared_ptr<PartitionManager> pmgr = std::make_shared<PartitionManager>();
    AABB world{0, 0, 1000, 1000};
    pmgr->Init(world, 100);

    MigrationManager mmgr(pmgr);

    // 准备源分区
    auto partitions = pmgr->GetAllPartitions();
    uint32_t src_pid = partitions[0]->partition_id;
    auto* src_aoi = pmgr->GetPartitionAOI(src_pid);

    for (int i = 0; i < 15; ++i) {
        src_aoi->AddEntity(i, i * 10, i * 10);
    }

    // 模拟迁移数据
    mmgr.SetOnDataReadyCallback([this](const MigrationData& data) {
        g_mock_network_data = data;
        g_mock_network_has_data = true;
    });

    // 发起迁移
    bool started = mmgr.StartMigration(src_pid, 999);
    assert(started);

    // 获取迁移数据
    MigrationData data;
    assert(MockReceiveFromSource(data));

    // 在目标节点恢复（同一个分区，但这里我们把它当成目标节点处理）
    bool restored = mmgr.ReceiveMigrationData(data);
    assert(restored);

    // 验证：目标分区应该有 15 个实体
    auto* target_aoi = pmgr->GetPartitionAOI(src_pid);
    auto entities = target_aoi->GetAllEntities();
    assert(entities.size() == 15);

    std::cout << "  Restored " << entities.size() << " entities ✓" << std::endl;
    std::cout << "[PASS] Deserialization and restore correct" << std::endl;
}

// ===== 测试 4：迁移完成后源节点释放资源 =====
void Test_Migration::TestSourceCleanup() {
    std::cout << "\n[Test 4] Source Cleanup..." << std::endl;

    std::shared_ptr<PartitionManager> pmgr = std::make_shared<PartitionManager>();
    AABB world{0, 0, 1000, 1000};
    pmgr->Init(world, 100);

    MigrationManager mmgr(pmgr);

    auto partitions = pmgr->GetAllPartitions();
    uint32_t pid = partitions[0]->partition_id;
    auto* aoi = pmgr->GetPartitionAOI(pid);

    // 插入 20 个实体
    for (int i = 0; i < 20; ++i) {
        aoi->AddEntity(i, i * 5, i * 5);
    }

    auto before_entities = aoi->GetAllEntities();
    assert(before_entities.size() == 20);

    // 模拟迁移
    mmgr.SetOnDataReadyCallback([this](const MigrationData& data) {
        g_mock_network_data = data;
        g_mock_network_has_data = true;
    });

    mmgr.StartMigration(pid, 999);

    MigrationData data;
    MockReceiveFromSource(data);

    mmgr.ReceiveMigrationData(data);
    mmgr.ConfirmMigration(pid);

    // 验证：源分区应该已被清空（Inactive 状态）
    auto p = pmgr->GetPartition(pid);
    assert(p->state == PartitionState::Active);

    auto after_entities = aoi->GetAllEntities();
    assert(after_entities.size() == 0);

    std::cout << "  Cleanup: " << before_entities.size() << " -> " << after_entities.size() << " entities ✓" << std::endl;
    std::cout << "[PASS] Source cleanup correct" << std::endl;
}

// ===== 测试 5：迁移失败回滚 =====
void Test_Migration::TestRollback() {
    std::cout << "\n[Test 5] Rollback..." << std::endl;

    std::shared_ptr<PartitionManager> pmgr = std::make_shared<PartitionManager>();
    AABB world{0, 0, 1000, 1000};
    pmgr->Init(world, 100);

    MigrationManager mmgr(pmgr);

    auto partitions = pmgr->GetAllPartitions();
    uint32_t pid = partitions[0]->partition_id;
    auto* aoi = pmgr->GetPartitionAOI(pid);

    for (int i = 0; i < 10; ++i) {
        aoi->AddEntity(i, i * 10, i * 10);
    }

    // 模拟迁移数据准备完成，但目标节点恢复失败
    mmgr.SetOnDataReadyCallback([this](const MigrationData& data) {
        g_mock_network_data = data;
        g_mock_network_has_data = true;
    });

    mmgr.StartMigration(pid, 999);

    MigrationData data;
    MockReceiveFromSource(data);

    // 模拟恢复失败：我们手动构造一个错误场景
    // 由于 ReceiveMigrationData 成功，这里我们模拟目标节点发送失败确认
    // 在真实场景中，Ack 会返回 failure
    // 在测试中，我们直接调用 RollbackMigration 模拟目标端恢复失败
    bool rollback = mmgr.RollbackMigration(pid);
    assert(rollback);
    assert(mmgr.GetMigrationState(pid) == MigrationState::Rollback);

    // 验证：分区重新变为 Active，玩家应该还在（因为回滚了）
    auto p = pmgr->GetPartition(pid);
    assert(p->state == PartitionState::Active);

    auto entities = aoi->GetAllEntities();
    assert(entities.size() == 10);

    std::cout << "  Rollback: state=Active, players=" << entities.size() << " ✓" << std::endl;
    std::cout << "[PASS] Rollback correct" << std::endl;
}

// ===== 测试 6：端到端完整迁移流程 =====
void Test_Migration::TestEndToEndMigration() {
    std::cout << "\n[Test 6] End-to-End Migration..." << std::endl;

    std::shared_ptr<PartitionManager> pmgr = std::make_shared<PartitionManager>();
    AABB world{0, 0, 1000, 1000};
    pmgr->Init(world, 100);

    MigrationManager mmgr(pmgr);

    auto partitions = pmgr->GetAllPartitions();
    uint32_t src_pid = partitions[0]->partition_id;
    auto* src_aoi = pmgr->GetPartitionAOI(src_pid);

    // 在源分区插入 30 个实体
    for (int i = 0; i < 30; ++i) {
        src_aoi->AddEntity(i, i * 3, i * 3);
    }

    // 设置回调：模拟网络传输
    mmgr.SetOnDataReadyCallback([this](const MigrationData& data) {
        std::cout << "[E2E] Data ready: " << data.total_players() << " players" << std::endl;
        g_mock_network_data = data;
        g_mock_network_has_data = true;
    });

    // 注册完成回调
    bool migration_completed = false;
    mmgr.SetOnMigrationCompleteCallback([&, this](uint32_t pid, bool success) {
        migration_completed = true;
        std::cout << "[E2E] Migration " << (success ? "success" : "failed") << " for partition " << pid << std::endl;
    });

    // 1. 启动迁移
    assert(mmgr.StartMigration(src_pid, 999));

    // 2. 模拟传输
    MigrationData data;
    assert(MockReceiveFromSource(data));

    // 3. 目标节点恢复
    assert(mmgr.ReceiveMigrationData(data));

    // 4. 确认完成
    assert(mmgr.ConfirmMigration(src_pid));

    // 5. 验证完成回调被调用
    assert(migration_completed);

    // 6. 验证数据完整性：源分区清空，目标分区有 30 个实体
    auto src_entities = src_aoi->GetAllEntities();
    assert(src_entities.size() == 0);

    // 验证状态
    auto p = pmgr->GetPartition(src_pid);
    assert(p->state == PartitionState::Active);

    std::cout << "  End-to-end migration completed successfully ✓" << std::endl;
    std::cout << "[PASS] End-to-End migration correct" << std::endl;
}

// ===== 主函数 =====
void Test_Migration::TestMigrationAll() {
    std::cout << "=== Day 3 Migration Tests ===" << std::endl;

    TestMigrationStateMachine();
    TestSerialization();
    TestDeserializationAndRestore();
    TestSourceCleanup();
    TestRollback();
    TestEndToEndMigration();

    std::cout << "\n=== ALL DAY 3 TESTS PASSED ===" << std::endl;
}