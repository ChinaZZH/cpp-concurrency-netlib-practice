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

    std::shared_ptr<MigrationManager> src_mmgr;
    MigrationData received_migration_data;
    {
        std::shared_ptr<PartitionManager> pmgr = std::make_shared<PartitionManager>();
        AABB world{0, 0, 1000, 1000};
        pmgr->Init(world, 100);
        
        auto partitions = pmgr->GetAllPartitions();
        uint32_t pid = partitions[0]->partition_id;
        // 插入一些测试实体
        auto* aoi = pmgr->GetPartitionAOI(pid);
        for(int i = 0; i < 10; ++i) {
            aoi->AddEntity(i, i * 10, i * 10);
        }

        src_mmgr = std::make_shared<MigrationManager>(pmgr);
        src_mmgr->SetOnDataReadyCallback([&received_migration_data](const MigrationData& data){
            received_migration_data = std::move(data);
        });

        // 1. 初始状态应为 Idle
        assert(src_mmgr->GetMigrationState(pid) == MigrationState::Idle);
        std::cout << "  Initial state: Idle ✓" << std::endl;

        // 2. 启动迁移
        bool started = src_mmgr->StartMigration(pid, 999);
        assert(started);
        assert(src_mmgr->GetMigrationState(pid) == MigrationState::Transferring);
        std::cout << "  After Start: Transferring ✓" << std::endl;
    }
    


    // 3. 模拟目标节点收到数据并恢复
    {
        std::shared_ptr<PartitionManager> pmgr = std::make_shared<PartitionManager>();
        AABB world{0, 0, 1000, 1000};
        pmgr->Init(world, 100);

        std::shared_ptr<MigrationManager> target_mmgr = std::make_shared<MigrationManager>(pmgr);
        bool restored = target_mmgr->ReceiveMigrationData(received_migration_data);
        assert(restored);
        assert(target_mmgr->GetMigrationState(received_migration_data.partition_id()) == MigrationState::Done);
        std::cout << "  After Receive: Done ✓" << std::endl;
    }

    //MigrationData received;
    //assert(MockReceiveFromSource(received));
    

    // 4. 确认迁移完成
    bool confirmed = src_mmgr->ConfirmMigration(received_migration_data.partition_id());
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

    std::shared_ptr<MigrationManager> src_mmgr;
    MigrationData received_migration_data;
    {
        std::shared_ptr<PartitionManager> pmgr = std::make_shared<PartitionManager>();
        AABB world{0, 0, 1000, 1000};
        pmgr->Init(world, 100);
        
        auto partitions = pmgr->GetAllPartitions();
        uint32_t pid = partitions[0]->partition_id;
        // 插入一些测试实体
        auto* aoi = pmgr->GetPartitionAOI(pid);
        for (int i = 0; i < 15; ++i) {
            aoi->AddEntity(i, i * 10, i * 10);
        }

        src_mmgr = std::make_shared<MigrationManager>(pmgr);
        src_mmgr->SetOnDataReadyCallback([&received_migration_data, this](const MigrationData& data){
            received_migration_data = std::move(data);
            g_mock_network_data = data;
            g_mock_network_has_data = true;
        });

        // 1. 初始状态应为 Idle
        assert(src_mmgr->GetMigrationState(pid) == MigrationState::Idle);
        std::cout << "  Initial state: Idle ✓" << std::endl;

        // 2. 启动迁移
        bool started = src_mmgr->StartMigration(pid, 999);
        assert(started);
        assert(src_mmgr->GetMigrationState(pid) == MigrationState::Transferring);
        std::cout << "  After Start: Transferring ✓" << std::endl;
    }

   
    // 3. 模拟目标节点收到数据并恢复
    {
        std::shared_ptr<PartitionManager> pmgr = std::make_shared<PartitionManager>();
        AABB world{0, 0, 1000, 1000};
        pmgr->Init(world, 100);

        std::shared_ptr<MigrationManager> target_mmgr = std::make_shared<MigrationManager>(pmgr);
        bool restored = target_mmgr->ReceiveMigrationData(received_migration_data);
        assert(restored);
       // 验证：目标分区应该有 15 个实体
        auto* target_aoi = pmgr->GetPartitionAOI(received_migration_data.partition_id());
        auto entities = target_aoi->GetAllEntities();
        assert(entities.size() == 15);

        std::cout << "  Restored " << entities.size() << " entities ✓" << std::endl;
    }
    
    
    std::cout << "[PASS] Deserialization and restore correct" << std::endl;
}

// ===== 测试 4：迁移完成后源节点释放资源 =====
void Test_Migration::TestSourceCleanup() {
    std::cout << "\n[Test 4] Source Cleanup..." << std::endl;

    std::shared_ptr<MigrationManager> src_mmgr;
    std::shared_ptr<PartitionManager> src_pmgr;
    MigrationData received_migration_data;
    std::vector<BaseEntityData> before_entities;

    {
        src_pmgr = std::make_shared<PartitionManager>();
        AABB world{0, 0, 1000, 1000};
        src_pmgr->Init(world, 100);
        
        auto partitions = src_pmgr->GetAllPartitions();
        uint32_t pid = partitions[0]->partition_id;
        // 插入一些测试实体
        auto* aoi = src_pmgr->GetPartitionAOI(pid);
        // 插入 20 个实体
        for (int i = 0; i < 20; ++i) {
            aoi->AddEntity(i, i * 5, i * 5);
        }

        before_entities = aoi->GetAllEntities();
        assert(before_entities.size() == 20);

        src_mmgr = std::make_shared<MigrationManager>(src_pmgr);
        // 模拟迁移
        src_mmgr->SetOnDataReadyCallback([&received_migration_data, this](const MigrationData& data){
            received_migration_data = std::move(data);
            g_mock_network_data = data;
            g_mock_network_has_data = true;
        });

        // 1. 初始状态应为 Idle
        assert(src_mmgr->GetMigrationState(pid) == MigrationState::Idle);
        std::cout << "  Initial state: Idle ✓" << std::endl;

        // 2. 启动迁移
        bool started = src_mmgr->StartMigration(pid, 999);
        assert(started);
        assert(src_mmgr->GetMigrationState(pid) == MigrationState::Transferring);
        std::cout << "  After Start: Transferring ✓" << std::endl;
    }

    

    // 3. 模拟目标节点收到数据并恢复
    {
        std::shared_ptr<PartitionManager> pmgr = std::make_shared<PartitionManager>();
        AABB world{0, 0, 1000, 1000};
        pmgr->Init(world, 100);

        std::shared_ptr<MigrationManager> target_mmgr = std::make_shared<MigrationManager>(pmgr);
        bool restored = target_mmgr->ReceiveMigrationData(received_migration_data);
        assert(restored);
        assert(target_mmgr->GetMigrationState(received_migration_data.partition_id()) == MigrationState::Done);
        std::cout << "  After Receive: Done ✓" << std::endl;
    }

   

    
    // 4. 确认迁移完成
    bool confirmed = src_mmgr->ConfirmMigration(received_migration_data.partition_id());
    assert(confirmed);
    
    
    // 验证：源分区应该已被清空（Inactive 状态）
    auto p = src_pmgr->GetPartition(received_migration_data.partition_id());
    assert(p->state == PartitionState::Inactive);

    auto after_entities = (p->aoi)->GetAllEntities();
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

    std::shared_ptr<MigrationManager> src_mmgr;
    MigrationData received_migration_data;
    bool migration_completed = false;
    IAOIManager* src_aoi = nullptr;
    std::shared_ptr<PartitionManager> src_pmgr;
    {
        std::shared_ptr<PartitionManager> pmgr = std::make_shared<PartitionManager>();
        AABB world{0, 0, 1000, 1000};
        pmgr->Init(world, 100);
        src_pmgr = pmgr;
        
        auto partitions = pmgr->GetAllPartitions();
        uint32_t pid = partitions[0]->partition_id;
        // 插入一些测试实体
        src_aoi = pmgr->GetPartitionAOI(pid);
        // 在源分区插入 30 个实体
        for (int i = 0; i < 30; ++i) {
            src_aoi->AddEntity(i, i * 3, i * 3);
        }

        src_mmgr = std::make_shared<MigrationManager>(pmgr);
        // 设置回调：模拟网络传输
        src_mmgr->SetOnDataReadyCallback([this, &received_migration_data](const MigrationData& data) {
            received_migration_data = std::move(data);

            std::cout << "[E2E] Data ready: " << data.total_players() << " players" << std::endl;
            g_mock_network_data = data;
            g_mock_network_has_data = true;
        });

        // 注册完成回调
        src_mmgr->SetOnMigrationCompleteCallback([&, this](uint32_t pid, bool success, std::string strErrorMsg) {
            migration_completed = true;
            std::cout << "[E2E] Migration " << (success ? "success" : "failed") << " for partition " << pid << std::endl;
        });

        // 1. 启动迁移
        assert(src_mmgr->StartMigration(pid, 999));
    }
    

    
    // 3. 模拟目标节点收到数据并恢复
    {
        std::shared_ptr<PartitionManager> pmgr = std::make_shared<PartitionManager>();
        AABB world{0, 0, 1000, 1000};
        pmgr->Init(world, 100);

        std::shared_ptr<MigrationManager> target_mmgr = std::make_shared<MigrationManager>(pmgr);
        bool restored = target_mmgr->ReceiveMigrationData(received_migration_data);
        assert(restored);
        assert(target_mmgr->GetMigrationState(received_migration_data.partition_id()) == MigrationState::Done);
        std::cout << "  After Receive: Done ✓" << std::endl;
    }
    

    // 4. 确认完成
    assert(src_mmgr->ConfirmMigration(received_migration_data.partition_id()));

    // 5. 验证完成回调被调用
    assert(migration_completed);

    // 6. 验证数据完整性：源分区清空，目标分区有 30 个实体
    auto src_entities = src_aoi->GetAllEntities();
    assert(src_entities.size() == 0);

    // 验证状态
    auto p = src_pmgr->GetPartition(received_migration_data.partition_id());
    assert(p->state == PartitionState::Inactive);

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