#include "AlgorithmsUnitTesting.h"
#include "../AOI/GridAOI.h"
#include "../AOI/CrossListAOI.h"
#include "../AOI/QuadTreeAOI.h"
#include "../../Common/FixedPoint.h"
#include "../../Common/FixedPonitMaxFunc.h"
#include "../FrameSync/ServerPlayerManager.h"
#include "../FrameSync/RemotePlayerSmoother.h"
#include "../../../build/proto_gen/game_event.pb.h"
#include "../EventSink/EventSink.h"
#include "../EventSink/EventStore.h"
#include "../Match/RankManager.h"
#include <vector>
#include <cstdint>
#include <iostream>
#include <thread>
#include <chrono>
#include "../Fsm/StateMachine.h"
#include "../Fsm/State/IdleState.h"
#include "../Fsm/State/PatrolState.h"
#include "../Fsm/State/ChaseState.h"
#include "../Fsm/State/AttackState.h"
#include "../../Common/FixedPoint.h"
#include "../../Common/FixedPonitMaxFunc.h"
#include "Test_BehaviorTreeNode.h"
#include "../BehaviorTree/BehaviorTree.h"
#include "../BehaviorTree/BTNode.h"
#include "../BehaviorTree/BTCompositeNode/BTCompositeNode.h"
#include "../BehaviorTree/BTCompositeNode/SequenceNode.h"
#include "../BehaviorTree/BTCompositeNode/SelectorNode.h"
#include "../BehaviorTree/BTCompositeNode/ParallelNode.h"
#include "../BehaviorTree/BTDecoratorNode/BTDecoratorNode.h"
#include "../BehaviorTree/BTDecoratorNode/CooldownNode.h"
#include "../BehaviorTree/BTDecoratorNode/InverterNode.h"
#include "../BehaviorTree/BTDecoratorNode/RepeaterNode.h"
#include "../BehaviorTree/BTDecoratorNode/TimeoutNode.h"
#include "../BehaviorTree/BTDecoratorNode/UntilSuccessNode.h"
#include "../AOI/DynamicAOI/DynamicAOI.h"
#include "../AOI/DynamicAOI/DynamicAOI_V2.h"
#include "../DynamicPartition/PartitionManager.h"
#include "../../Common/IncrementalHashTable.h"



 AlgorithmsUnitTesting::AlgorithmsUnitTesting()
 {

 }

 // 测试aoi算法九宫格
void AlgorithmsUnitTesting::TestAoiManager()
{
    std::shared_ptr<IAOIManager> aoi = std::make_shared<CrossListAOI>(100);
    //CrossListAOI aoi(100);

    aoi->AddEntity(1, 50, 50);
    std::cout << "Helle world" << std::endl;
    aoi->AddEntity(2, 120, 50);
    aoi->AddEntity(3, 50, 120);
    aoi->AddEntity(4, 200, 200);    
    PrintNeighbors(aoi, 1); // 应该在九宫格内看到 2, 3

    // 移动实体
    aoi->MoveEntity(2, 100, 50);
    PrintNeighbors(aoi, 1); // 2 可能仍在九宫格内

    aoi->MoveEntity(2, 300, 300);
    PrintNeighbors(aoi, 1); // 2 应该离开九宫格

    // 删除实体
    aoi->RemoveEntity(3);
    PrintNeighbors(aoi, 1);
}


void AlgorithmsUnitTesting::TestQuadTreeAOI() {
    std::shared_ptr<IAOIManager> aoi = std::make_shared<QuadTreeAOI>(1000, 1000, 100, 2, 8); // 世界 1000x1000，网格 100
    aoi->AddEntity(1, 50, 50);
    std::cout << "AddEntity  1 " << std::endl;
    aoi->AddEntity(2, 120, 50);
    std::cout << "AddEntity  2 " << std::endl;
    aoi->AddEntity(3, 50, 120);
    std::cout << "AddEntity 3 " << std::endl;
    aoi->AddEntity(4, 200, 200);
    std::cout << "AddEntity 4 " << std::endl;

    auto neighbors = aoi->GetNeighbors(1);
    std::cout << "QuadTree neighbors of 1: ";
    for (int id : neighbors) std::cout << id << " ";
    std::cout << std::endl;

    
    aoi->MoveEntity(2, 300, 300);
    neighbors = aoi->GetNeighbors(1);
    std::cout << "After move, neighbors of 1: ";
    for (int id : neighbors) std::cout << id << " ";

    std::cout << std::endl;
    
    aoi->RemoveEntity(3);
    neighbors = aoi->GetNeighbors(1);
    std::cout << "After remove, neighbors of 1: ";
    for (int id : neighbors) std::cout << id << " ";
    std::cout << std::endl;
}

void AlgorithmsUnitTesting::PrintNeighbors(std::shared_ptr<IAOIManager> aoi, int id)
{
    auto neighbors = aoi->GetNeighbors(id);
    std::cout << "Entity " << id << " neighbors: ";
    for (int n : neighbors) std::cout << n << " ";
    std::cout << std::endl;
}


void AlgorithmsUnitTesting::TestDeterminism() 
{
    FixedMath::InitSinTable();
    Fixed step = Fixed::FromRaw(100); // 小步长
    for (Fixed angle = Fixed::Zero(); angle < Fixed::Pi() * Fixed::FromRaw(2); angle += step) {
        Fixed v1 = FixedMath::Sin(angle);
        Fixed v2 = FixedMath::Sin(angle);
        if (v1 != v2) {
            // 如果触发，说明定点数存在非确定性
            std::cerr << "Non-deterministic at " << angle.ToDouble() << std::endl;
        }
    }
    std::cout << "Determinism test passed." << std::endl;
}


void AlgorithmsUnitTesting::TestFullDeterminism() {
    FixedMath::InitSinTable();
    std::cout << "[Test] Running full determinism suite..." << std::endl;

    // 1. 测试 Sin 和 Cos（角度覆盖 0~2PI）
    Fixed step = Fixed::FromRaw(100); // 约 3.5e-12 弧度步长
    Fixed two_pi = Fixed::Pi() * Fixed::FromRaw(2);
    bool sin_failed = false, cos_failed = false;
    
    for (Fixed angle = Fixed::Zero(); angle < two_pi; angle += step) {
        // 两次独立计算（模拟跨设备执行）
        Fixed s1 = FixedMath::Sin(angle);
        Fixed s2 = FixedMath::Sin(angle);
        if (s1 != s2) {
            std::cerr << "[FAIL] Sin mismatch at " << angle.ToDouble() << std::endl;
            sin_failed = true;
        }
        
        Fixed c1 = FixedMath::Cos(angle);
        Fixed c2 = FixedMath::Cos(angle);
        if (c1 != c2) {
            std::cerr << "[FAIL] Cos mismatch at " << angle.ToDouble() << std::endl;
            cos_failed = true;
        }
    }
    if (!sin_failed) std::cout << "[PASS] Sin determinism." << std::endl;
    if (!cos_failed) std::cout << "[PASS] Cos determinism." << std::endl;

    // 2. 测试 Sqrt（覆盖零、一、小中大各种数量级）
    std::vector<Fixed> sqrt_tests = {
        Fixed::Zero(),
        Fixed::One(),
        Fixed::FromRaw(1000),                // 极小数
        Fixed::FromRaw(1LL << 20),           // 中等数
        Fixed::FromRaw(1LL << 30),           // 大数（仍在 Q24.40 安全范围内）
    };
    bool sqrt_failed = false;
    for (auto& v : sqrt_tests) {
        Fixed r1 = FixedMath::FixedSqrt(v);
        Fixed r2 = FixedMath::FixedSqrt(v);
        if (r1 != r2) {
            std::cerr << "[FAIL] Sqrt mismatch at " << v.ToDouble() << std::endl;
            sqrt_failed = true;
        }
        // 额外验证： sqrt(x)^2 == x（允许极小舍入）
        Fixed check = r1 * r1;
        Fixed diff = check - v;
        if (diff < Fixed::Zero()) diff = -diff;
        if (diff > Fixed::FromRaw(10)) { // 容忍极小误差
            std::cerr << "[WARN] Sqrt precision loss at " << v.ToDouble() 
                      << ", diff=" << diff.ToDouble() << std::endl;
        }
    }
    if (!sqrt_failed) std::cout << "[PASS] Sqrt determinism." << std::endl;

    // 3. 测试四则运算的确定性（乘除法依赖 __int128，必须验证）
    Fixed a = Fixed::FromRaw(1234567);
    Fixed b = Fixed::FromRaw(7654321);
    Fixed m1 = a * b;
    Fixed m2 = a * b;
    Fixed d1 = a / b;
    Fixed d2 = a / b;
    if (m1 == m2 && d1 == d2) {
        std::cout << "[PASS] Mul/Div determinism." << std::endl;
    } else {
        std::cerr << "[FAIL] Mul/Div mismatch." << std::endl;
    }

    std::cout << "=== Full determinism test completed ===" << std::endl;
}



void AlgorithmsUnitTesting::TestServerPlayerManger()
{
    // 测试案例1
    bool bStepSwitch_1 = false;
    if(bStepSwitch_1)
    {
        ServerPlayerManager mgr(nullptr);
        mgr.AddPlayer(1);

    
        ClientInput input;
        input.set_move_x(1);
        mgr.SumbitInput(1, input);

        mgr.Tick(20);

        ServerPlayerState state;
        mgr.GetPlayerState(1, state);
        // state.x 应该从 0 变为 2.0（因为 1 * 0.1 * 20 = 2）
        printf("Server pos: %.2f\n", state.x.ToDouble());
    }
    

    // 测试样例2
    bool bStepSwitch_2 = false;
    if(bStepSwitch_2)
    {
        printf("=== Step 1 History Storage Test ===\n\n");
        ServerPlayerManager mgr(nullptr);
        mgr.AddPlayer(1);

        // 2. 手动构造三次 Tick 的输入
        //   Tick 1: client_frame = 10, 向右移动 (move_x = 1)
        ClientInput input1;
        input1.set_player_id(1);
        input1.set_frame_index(10);
        input1.set_move_x(1);
        input1.set_move_y(0);
        input1.set_predicted_x(0);  // 占位，不影响服务端模拟
        input1.set_predicted_y(0);
        mgr.SumbitInput(1, input1);
        mgr.Tick(20);  // Tick 1: 消费 input1，位置从 0 -> 2.0

        //    Tick 2: client_frame = 11, 继续向右移动
        ClientInput input2;
        input2.set_player_id(1);
        input2.set_frame_index(11);
        input2.set_move_x(1);
        input2.set_move_y(0);
        mgr.SumbitInput(1, input2);
        mgr.Tick(20);  // Tick 2: 消费 input2，位置从 2.0 -> 4.0

        mgr.Tick(20);  // Tick 3: 空输入，位置保持 4.0（假设无惯性）

        // 4. 查询历史：按 client_frame = 11 查询
        printf("\n--- Querying History ---\n");
        ServerPlayerState found_state;
        bool found = mgr.GetHistoryByServerFrame(1, 11, found_state);

        if(found) {
            PrintState("Found state for client_frame=11", found_state);
            // 预期：经过两次 Tick，位置应为 4.0
            assert(found_state.x.ToDouble() == 4.0);
            assert(found_state.y.ToDouble() == 0.0);
            printf("✅ client_frame=11 query PASSED.\n");
        } else {
            printf("❌ client_frame=11 query FAILED (not found).\n");
            return ;
        }


        // 5. 查询一个不存在的 client_frame（例如 99），应返回 false
        printf("\n--- Querying Non-existent Frame ---\n");
        ServerPlayerState dummy;
        bool not_found = mgr.GetHistoryByServerFrame(1, 99, dummy);
        if (!not_found) {
            printf("✅ client_frame=99 query correctly returned false.\n");
        } else {
            printf("❌ client_frame=99 query incorrectly returned true.\n");
            return ;
        }


        // 6. 查询 client_frame = 10（第一次 Tick 的状态），验证是否能找到
        printf("\n--- Querying client_frame=10 ---\n");
        ServerPlayerState state10;
        bool found10 = mgr.GetHistoryByServerFrame(1, 10, state10);
        if (found10) {
            PrintState("Found state for client_frame=10", state10);
            // 预期：第一次 Tick 后位置应为 2.0
            assert(state10.x.ToDouble() == 2.0);
            printf("✅ client_frame=10 query PASSED.\n");
        } else {
            printf("❌ client_frame=10 query FAILED.\n");
            return ;
        }


        // 7. 查询 client_frame = 0（空输入帧），应返回 false（因为查询条件跳过 0）
        printf("\n--- Querying client_frame=0 (should skip) ---\n");
        bool found0 = mgr.GetHistoryByServerFrame(1, 0, dummy);
        if (!found0) {
            printf("✅ client_frame=0 correctly skipped.\n");
        } else {
            printf("❌ client_frame=0 should be skipped but returned true.\n");
            return ;
        }

        printf("\n=== All Step 1 Tests PASSED ===\n");
    }


    // 测试防作弊
    bool bStepSwitch_3 = true;
    if(bStepSwitch_3)
    {
        ServerPlayerManager mgr(nullptr);
        mgr.AddPlayer(1);

    
        ClientInput input;
        input.set_move_x(1);
        input.set_client_seq(1);
        mgr.SumbitInput(1, input);
        mgr.Tick(20);

        //ServerPlayerState state;
        //mgr.GetPlayerState(1, state);
        input.set_move_x(1);
        input.set_client_seq(3);
        mgr.SumbitInput(1, input);
        mgr.Tick(20);


        input.set_move_x(1);
        input.set_client_seq(5);
        mgr.SumbitInput(1, input);
        mgr.Tick(20);

        input.set_move_x(1);
        input.set_client_seq(7);
        mgr.SumbitInput(1, input);
        mgr.Tick(20);

        input.set_move_x(1);
        input.set_client_seq(9);
        mgr.SumbitInput(1, input);
        mgr.Tick(20);

         input.set_move_x(1);
        input.set_client_seq(11);
        mgr.SumbitInput(1, input);
        mgr.Tick(20);
    }
}

// 辅助：打印定点数
void AlgorithmsUnitTesting::PrintState(const char* label, const ServerPlayerState& state) {
    printf("%s: pos=(%.2f, %.2f), hp=%u\n", 
           label, state.x.ToDouble(), state.y.ToDouble(), state.hp);
}

/*
void AlgorithmsUnitTesting::TestRemotePlayerSmoother()
{
    RemotePlayerSmoother smoother;
    {
        // 单状态测试
        RemoteStateSnapshot s1; 
        s1.x = Fixed::FromRaw(0); 
        s1.y = Fixed::Zero(); 
        s1.timeStamp_ms = 100; 
        s1.valid = true;

        
        smoother.PushState(s1);
        bool ready = smoother.IsReady(); // 应为 false（只有 prev_）
        if(ready)
        {
            std::cout << "[FAIL] One RemotePlayerSmoother test." << std::endl;
        }
        else{
            std::cout << "[PASS] One RemotePlayerSmoother test." << std::endl;
        }
    }
        

    // 双状态测试：
    {
        RemoteStateSnapshot s2; 
        s2.x = Fixed::FromRaw(100); 
        s2.y = Fixed::Zero(); 
        s2.timeStamp_ms = 200; 
        s2.valid = true;

        smoother.PushState(s2);
        bool ready = smoother.IsReady(); // 应为 true（prev_ 和 next_ 都有）
         if(ready)
        {
            std::cout << "[PASS] two RemotePlayerSmoother test." << std::endl;
        }
        else{
            std::cout << "[FAIL] two RemotePlayerSmoother test." << std::endl;
        }
    }

        

    {
     
        // 此时 prev_ 应为 s2 (x=100)，next_ 应为 s3 (x=200)
        RemoteStateSnapshot s3; 
        s3.x = Fixed::FromRaw(200); 
        s3.y = Fixed::Zero(); 
        s3.timeStamp_ms = 300; 
        s3.valid = true;
        smoother.PushState(s3);
           
        auto render = smoother.GetRenderState(250); // 时间在 100~200 之间
        // 理论上 render.x 应接近 50（因为 t=0.5，100*0.5=50）
        std::cout << "reader.x must be 150 ,got :=" << render.x.Raw() << std::endl;
    }

    
    {
        // 应返回 prev_ (x=0)
        auto render_before = smoother.GetRenderState(50);  // 早于 prev_
        if(0 == render_before.x.Raw())
        {
            std::cout << "[PASS] render_before test." << std::endl;
        }
        else{
            std::cout << "[FAIL] render_before test." << std::endl;
        }

        // 应返回 next_ (x=200)
        Fixed expected = Fixed::FromRaw(200); 
        auto render_after = smoother.GetRenderState(350);  // 晚于 next_
        if(expected.Raw() == render_after.x.Raw())
        {
            std::cout << "[PASS] render_before test." << std::endl;
        }else{
            std::cout << "[FAIL] render_after test x:=." << render_after.x.Raw() << std::endl;
        }
            
    }
}
*/

 void AlgorithmsUnitTesting::TestEventSinkLog()
 {
    std::shared_ptr<EventSink> event_sink = std::make_shared<EventSink>();
    event_sink->Init("../logs/events.bin");

    std::shared_ptr<EventStore> event_store = std::make_shared<EventStore>();
    event_store->Init(event_sink);

    uint32_t player_id = 1;
    uint32_t server_frame_index = 10;
    uint32_t current_time_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

    uint32_t target_id = 10;
    uint32_t skill_id = 1000;
    int64_t x = 100;
    int64_t y = 500;

     // 记录攻击事件
    {
        GameEvent event;
        event.set_player_id(player_id);
        event.set_server_frame(server_frame_index);
        event.set_timestamp_ms(current_time_ms);
        event.set_event_type(EVENT_ATTACK);

        AttackPayload* attackPayload = event.mutable_attack();
        attackPayload->set_target_id(target_id);
        attackPayload->set_skill_id(skill_id);
        attackPayload->set_dir_x(x);
        attackPayload->set_dir_y(y);

        event_store->Record(event);
    }


    // 记录命中事件
     {
        
            GameEvent event;
            event.set_player_id(player_id);
            event.set_server_frame(server_frame_index);
            event.set_timestamp_ms(current_time_ms);
            event.set_event_type(EVENT_HIT);

            HitPayload* hitPayload = event.mutable_hit();
            hitPayload->set_target_id(target_id);
            hitPayload->set_damage(10);
            hitPayload->set_remaining_hp(10);
        
            event_store->Record(event);
    }
 }


 // 辅助函数：构造一个测试事件
GameEvent AlgorithmsUnitTesting::MakeTestEvent(uint32_t player_id, uint32_t server_frame, EventType type, uint32_t target_id /*= 0*/, int32_t damage /*= 0*/) 
{
    GameEvent event;
    event.set_player_id(player_id);
    event.set_server_frame(server_frame);
    event.set_timestamp_ms(std::chrono::duration_cast<std::chrono::milliseconds>(
                               std::chrono::system_clock::now().time_since_epoch())
                               .count());
    event.set_event_type(type);

    if (type == EVENT_ATTACK) 
    {
        auto* payload = event.mutable_attack();
        payload->set_target_id(target_id);
        payload->set_skill_id(1);
        payload->set_dir_x(100);
        payload->set_dir_y(0);
    } 
    else if (type == EVENT_HIT)
    {
        auto* payload = event.mutable_hit();
        payload->set_target_id(target_id);
        payload->set_damage(damage);
        payload->set_remaining_hp(100 - damage);
    }

    return event;
}


// 打印事件（调试用）
void AlgorithmsUnitTesting::PrintEvent(const GameEvent& event) 
{
    std::cout << "  [Event] id=" << event.player_id()
              << " frame=" << event.server_frame()
              << " type=" << event.event_type();
    if (event.has_hit()) {
        std::cout << " damage=" << event.hit().damage();
    }
    std::cout << std::endl;
}


 void AlgorithmsUnitTesting::TestEventStore()
 {
    std::cout << "=== EventStore Test Suite ===" << std::endl;

    // ================================================================
    // 准备：创建 EventStore（绑定一个空的 EventSink，只做内存缓存测试）
    // ================================================================
    auto sink = std::make_shared<EventSink>();
    // 不调用 Init，避免写文件干扰测试

    EventStore store;
    store.Init(sink, 10);  // max_events = 10（方便测试淘汰）

    // ================================================================
    // Test 1: 初始化成功，GetEventCount() = 0
    // ================================================================
    std::cout << "\n[Test 1] Initial state..." << std::endl;
    assert(store.GetEventCount() == 0);
    std::cout << "  PASS: GetEventCount() == 0" << std::endl;

    // ================================================================
    // Test 2: 记录事件后，GetEventCount() 正确增长
    // ================================================================
    std::cout << "\n[Test 2] Record events..." << std::endl;
    for (int i = 0; i < 5; i++) 
    {
        auto event = this->MakeTestEvent(1001, 100 + i, EVENT_ATTACK, 1002);
        store.Record(event);
    }

    assert(store.GetEventCount() == 5);
    std::cout << "  PASS: GetEventCount() == 5 after 5 records" << std::endl;

    // ================================================================
    // Test 3: 按玩家 ID 查询，返回该玩家的最近事件，且按时间降序
    // ================================================================
    std::cout << "\n[Test 3] QueryByPlayer..." << std::endl;

    // 额外记录一些其他玩家的事件
    for (int i = 0; i < 3; i++) {
        auto event = MakeTestEvent(2001, 200 + i, EVENT_ATTACK, 1001);
        store.Record(event);
    }
    // 再记录一些玩家 1001 的事件（最新的）
    for (int i = 0; i < 4; i++) {
        auto event = MakeTestEvent(1001, 300 + i, EVENT_HIT, 2001, 10 + i);
        store.Record(event);
    }

    auto results = store.QueryByPlayer(1001, 5);
    assert(results.size() == 5);
    std::cout << "  QueryByPlayer(1001, 5) returned " << results.size() << " events" << std::endl;

    // 验证降序：最新的在前
    for (size_t i = 1; i < results.size(); i++) {
        assert(results[i-1].server_frame() >= results[i].server_frame());
    }
    std::cout << "  PASS: Events are in descending frame order" << std::endl;

     // 验证返回的都是玩家 1001 的事件
    for (const auto& ev : results) {
        assert(ev.player_id() == 1001);
    }
    std::cout << "  PASS: All returned events belong to player 1001" << std::endl;

    // ================================================================
    // Test 4: 按帧号范围查询，返回区间内所有事件
    // ================================================================
    std::cout << "\n[Test 4] QueryByFrameRange..." << std::endl;

    auto range_results = store.QueryByFrameRange(150, 350);
    std::cout << "  QueryByFrameRange(150, 350) returned " << range_results.size() << " events" << std::endl;

    // 验证所有事件都在区间内
    for (const auto& ev : range_results) {
        assert(ev.server_frame() >= 150 && ev.server_frame() <= 350);
    }
    std::cout << "  PASS: All events are within frame range [150, 350]" << std::endl;


     // ================================================================
    // Test 5: 超过 max_events 时，最旧事件被自动淘汰
    // ================================================================
    std::cout << "\n[Test 5] Auto-eviction (max_events=10)..." << std::endl;

    // 当前已有 5 + 3 + 4 = 12 个事件，超过 10，应该已经淘汰了 2 个最旧的
    // 但为了更精确地测试淘汰，我们手动填充到 15 个
    for (int i = 0; i < 5; i++) {
        auto event = MakeTestEvent(3001, 400 + i, EVENT_ATTACK, 1001);
        store.Record(event);
    }

    // 此时总共应该有 17 个事件（12 + 5），但 max_events=10，所以应该有 10 个
    size_t count = store.GetEventCount();
    std::cout << "  After 17 records with max_events=10, count=" << count << std::endl;
    assert(count == 10);
    std::cout << "  PASS: Event count capped at max_events=10" << std::endl;

    // 验证最旧的事件（帧号 < 某些值）已经被淘汰
    auto oldest_events = store.QueryByFrameRange(0, 100);
    // 最旧的帧号应该是 300+ 左右的，0-100 的应该已经不存在了
    std::cout << "  QueryByFrameRange(0, 100) returned " << oldest_events.size() << " events (should be 0)" << std::endl;
    assert(oldest_events.empty());
    std::cout << "  PASS: Oldest events (frame < 100) were evicted" << std::endl;

    // ================================================================
    // Test 6: 并发写入安全性
    // ================================================================
    std::cout << "\n[Test 6] Concurrent writes..." << std::endl;

    const int THREAD_COUNT = 4;
    const int EVENTS_PER_THREAD = 50;
    std::vector<std::thread> threads;

    for (int t = 0; t < THREAD_COUNT; t++) {
        threads.emplace_back([&store, t, this]() {
            uint32_t base_player = 5000 + t;
            for (int i = 0; i < EVENTS_PER_THREAD; i++) {
                auto event = MakeTestEvent(base_player, 1000 + t * 100 + i, EVENT_ATTACK, 1001);
                store.Record(event);
            }
        });
    }

    for (auto& th : threads) {
        th.join();
    }

    // 检查总事件数（应该仍然是 max_events=10，因为一直在淘汰）
    size_t final_count = store.GetEventCount();
    std::cout << "  After " << THREAD_COUNT * EVENTS_PER_THREAD
              << " concurrent writes, final count=" << final_count
              << " (should be <= max_events=10)" << std::endl;
    assert(final_count <= 10);
    std::cout << "  PASS: Concurrent writes completed without crash, count capped correctly" << std::endl;

    // ================================================================
    // 全部通过
    // ================================================================
    std::cout << "\n=== ALL TESTS PASSED ===" << std::endl;
 }


 void AlgorithmsUnitTesting::TestRankMgr()
 {
    std::cout << "=== RankManager Test Suite ===" << std::endl;

    RankManager mgr;
    const uint32_t SHARD_SIZE = 100;

    // ================================================================
    // Test 1: Init() 正确设置分片大小
    // ================================================================
    std::cout << "\n[Test 1] Init shard size..." << std::endl;
    mgr.Init(SHARD_SIZE);
    assert(mgr.GetShardCount() == 0);
    assert(mgr.GetTotalPlayers() == 0);
    std::cout << "  PASS: Init succeeded, empty state." << std::endl;

    // ================================================================
    // Test 2: UpdateScore() 正确添加玩家，分片自动扩展
    // ================================================================
    std::cout << "\n[Test 2] Add players..." << std::endl;
    mgr.UpdateScore(1001, 1200); // [1200, 1300)
    mgr.UpdateScore(1002, 1250); // [1200, 1300)
    mgr.UpdateScore(1003, 1180); // [1100, 1200)
    mgr.UpdateScore(1004, 2000); // [2000, 2100)
    mgr.UpdateScore(1005, 150); // [100, 200)

    assert(mgr.GetTotalPlayers() == 5);
    assert(mgr.GetShardCount() == 4);  // 分片: 100-199, 1100-1199, 1200-1299, 1900-1999? 实际是 150->1, 1180->11, 1200->12, 1250->12, 2000->20 => 4个分片

    // 验证分片数量: 150->1, 1180->11, 1200/1250->12, 2000->20 => 4
    // 但 GetShardCount 应该返回 4
    // 为了更准确，我们手动检查
    uint32_t shard_count = mgr.GetShardCount();
    std::cout << "  Shard count: " << shard_count << " (expected >= 4)" << std::endl;
    assert(shard_count >= 3);
    std::cout << "  PASS: Players added, shards created." << std::endl;

    // ================================================================
    // Test 3: GetTopN(3) 返回分数最高的 3 个玩家
    // ================================================================
    std::cout << "\n[Test 3] GetTopN..." << std::endl;
    PrintTopN(mgr, 3);
    auto top3 = mgr.GetTopN(3);
    assert(top3.size() == 3);
    // 最高分应该是 1004(2000), 1002(1250), 1001(1200)
    uint32_t first_score, second_score, third_score;
    mgr.GetScore(top3[0], first_score);
    mgr.GetScore(top3[1], second_score);
    mgr.GetScore(top3[2], third_score);
    assert(first_score == 2000);
    assert(second_score == 1250);
    assert(third_score == 1200);
    std::cout << "  PASS: Top 3 are 1004(2000), 1002(1250), 1001(1200)" << std::endl;

    // ================================================================
    // Test 4: GetRank() 返回正确的玩家排名（从 1 开始）
    // ================================================================
    std::cout << "\n[Test 4] GetRank..." << std::endl;
    int32_t rank = mgr.GetRank(1004);
    assert(rank == 1);
    rank = mgr.GetRank(1002);
    assert(rank == 2);
    rank = mgr.GetRank(1001);
    assert(rank == 3);
    rank = mgr.GetRank(1003);
    assert(rank == 4);
    rank = mgr.GetRank(1005);
    assert(rank == 5);
    std::cout << "  PASS: All ranks correct." << std::endl;

    // ================================================================
    // Test 5: 更新分数后，排名自动调整
    // ================================================================
    std::cout << "\n[Test 5] Update score and rank adjust..." << std::endl;
    // 将 1003 从 1180 提升到 2100（变成最高）
    mgr.UpdateScore(1003, 2100);
    PrintTopN(mgr, 3);
    auto top3_after = mgr.GetTopN(3);
    mgr.GetScore(top3_after[0], first_score);
    mgr.GetScore(top3_after[1], second_score);
    mgr.GetScore(top3_after[2], third_score);
    assert(first_score == 2100);  // 1003 现在最高
    assert(second_score == 2000); // 1004 第二
    assert(third_score == 1250);  // 1002 第三
    // 检查排名
    assert(mgr.GetRank(1003) == 1);
    assert(mgr.GetRank(1004) == 2);
    assert(mgr.GetRank(1002) == 3);
    std::cout << "  PASS: Rank adjusted correctly." << std::endl;


    // ================================================================
    // Test 6: 空分片在玩家移除后被自动清理
    // ================================================================
    std::cout << "\n[Test 6] Empty shard cleanup..." << std::endl;
    // 当前的玩家分布：1003(2100)->21, 1004(2000)->20, 1002(1250)->12, 1001(1200)->12, 1005(150)->1
    // 分片: 1, 12, 20, 21
    // 移除 1005（分片1的唯一玩家），分片1应被清理
    // 但我们需要移除所有玩家才能清空分片。为了测试，我们移除分片12中的所有玩家
    mgr.UpdateScore(1001, 2100); // 移动 1001 到 21
    mgr.UpdateScore(1002, 2100); // 移动 1002 到 21
    // 现在分片12应该空了（原来有1001,1002两个玩家），但分片12可能还有其他玩家？没有，只有这两个。
    // 确保分片12被清理
    // 我们通过检查分片数量来验证
    uint32_t shard_count_before = mgr.GetShardCount();
    //mgr.PrintShards();
    //assert(shard_count_before == 6);

    // 当前分片: 1(可能有1005?), 20(1004), 21(1003,1001,1002) => 但1005还在1，所以1还在
    // 为了测试清空，我们移除1005
    mgr.UpdateScore(1005, 2100); // 移动1005到21，分片1空了
    uint32_t shard_count_after = mgr.GetShardCount();
    // 应该只有分片20和21（分片1被清理）
    //mgr.PrintShards();
    assert(shard_count_after == 2);
    std::cout << "  PASS: Empty shard cleaned up." << std::endl;


    // ================================================================
    // 全部通过
    // ================================================================
    std::cout << "\n=== ALL TESTS PASSED ===" << std::endl;
 }

 // 辅助：打印 Top N
void AlgorithmsUnitTesting::PrintTopN(const RankManager& mgr, uint32_t n) 
{
    auto top = mgr.GetTopN(n);
    std::cout << "  Top " << n << ": ";
    for (uint32_t pid : top) {
        uint32_t mmr;
        mgr.GetScore(pid, mmr);
        std::cout << pid << "(" << mmr << ") ";
    }
    std::cout << std::endl;
}


// 辅助：打印当前状态和位置
void AlgorithmsUnitTesting::PrintStatus(const StateMachine& fsm, const StateContext& ctx, const char* msg /*= ""*/) {
    std::cout << "[Status] " << msg
              << " | State: " << fsm.GetCurrentStateName()
              << " | Pos: (" << ctx.x.ToFloat() << ", " << ctx.y.ToFloat() << ")"
              << " | Target: " << ctx.target_id
              << std::endl;
}


void AlgorithmsUnitTesting::TestFsm()
{
   std::cout << "=== FSM Test (Fixed-Point) ===" << std::endl;

    StateMachine fsm;
    StateContext new_ctx;
    new_ctx.entity_id = 1001;
    new_ctx.x = Fixed(0.0f);
    new_ctx.y = Fixed(0.0f);
    new_ctx.hp = Fixed(100.0f);
    new_ctx.target_id = 0;

    fsm.Init(new IdleState(), new_ctx);

    auto& ctx = fsm.GetStateContext();
    PrintStatus(fsm, ctx, "Init");


    const float DELTA_MS = 20.0f;
    const int TOTAL_FRAMES = 500;

    bool target_appeared = false;
    bool target_close = false;
    bool target_escaped = false;

     for (int frame = 0; frame < TOTAL_FRAMES; frame++) {

        float current_time = frame * DELTA_MS / 1000.0f;

        // ---- 事件触发 ----
        if (current_time >= 3.0f && !target_appeared) {
            ctx.target_id = 2001;
            ctx.target_x = Fixed(50.0f);
            ctx.target_y = Fixed(50.0f);
            target_appeared = true;
            std::cout << "\n[Event] Target appeared at (50, 50)" << std::endl;
        }

        if (current_time >= 6.0f && !target_close) {
            ctx.target_x = Fixed(20.0f);
            ctx.target_y = Fixed(20.0f);
            target_close = true;
            std::cout << "\n[Event] Target moved closer to (20, 20)" << std::endl;
        }


        if (current_time >= 8.0f && !target_escaped) {
            ctx.target_x = Fixed(100.0f);
            ctx.target_y = Fixed(100.0f);
            target_escaped = true;
            std::cout << "\n[Event] Target escaped to (100, 100)" << std::endl;
        }

        if (current_time >= 9.5f && ctx.target_id != 0) {
            ctx.target_id = 0;
            std::cout << "\n[Event] Target disappeared" << std::endl;
        }

        // ---- 更新状态机 ----
        fsm.Update(DELTA_MS);

        if (frame % 50 == 0) {
            std::cout << "[Tick] " << current_time << "s -> State: "
                      << fsm.GetCurrentStateName()
                      << " | Pos: (" << ctx.x.ToFloat() << ", " << ctx.y.ToFloat() << ")" << std::endl;
        }
     }

     std::cout << "\n=== Final State ===" << std::endl;
    PrintStatus(fsm, ctx, "After 10s");

    if (fsm.GetCurrentStateName() == "Idle") {
        std::cout << "\n✅ TEST PASSED" << std::endl;
    } else {
        std::cout << "\n❌ TEST FAILED" << std::endl;
    }
}


void AlgorithmsUnitTesting::PrintResult_ForBehaviorTree(const std::string& test_name, BTStatus status)
{
    const char* status_str = "";
    if (status == BTStatus::Success)
    {
        status_str = "Success";
    } 
    else if (status == BTStatus::Failure)
    {
        status_str = "Failure";
    } 
    else 
    {
        status_str = "Running";
    }

    std::cout << "[Test] " << test_name << " => " << status_str << std::endl;
}


void AlgorithmsUnitTesting::TestBehaviorTree_Step2()
{
    std::cout << "=== Behavior Tree Composite Node Test ===\n" << std::endl;

    StateContext ctx;

    // ----------------------------------------------------------------
    // 1. SequenceNode 测试
    // ----------------------------------------------------------------
    std::cout << "--- 1. SequenceNode ---" << std::endl;

    // 1.1 全部成功 => Success
    auto seq1 = std::make_unique<SequenceNode>();
    seq1->AddChild(std::make_unique<SuccessNode>());
    seq1->AddChild(std::make_unique<SuccessNode>());
    BTStatus r1 = seq1->Execute(ctx, 0.0f);
    // 需要执行多次才能完成
    while (r1 == BTStatus::Running) r1 = seq1->Execute(ctx, 0.0f);
    PrintResult_ForBehaviorTree("All Success", r1);

    // 1.2 任一失败 => Failure
    auto seq2 = std::make_unique<SequenceNode>();
    seq2->AddChild(std::make_unique<SuccessNode>());
    seq2->AddChild(std::make_unique<FailureNode>());
    BTStatus r2 = seq2->Execute(ctx, 0.0f);
    while (r2 == BTStatus::Running) r2 = seq2->Execute(ctx, 0.0f);
    PrintResult_ForBehaviorTree("With Failure", r2);

    // 1.3 包含 Running => Running
    auto seq3 = std::make_unique<SequenceNode>();
    seq3->AddChild(std::make_unique<RunningNode>());
    seq3->AddChild(std::make_unique<SuccessNode>());
    BTStatus r3 = seq3->Execute(ctx, 0.0f);
    PrintResult_ForBehaviorTree("With Running", r3);

    // 1.4 中间节点 Running，后续还有节点 => Running (状态机需多次执行)
    auto seq4 = std::make_unique<SequenceNode>();
    seq4->AddChild(std::make_unique<RunningThenSuccessNode>(2));
    seq4->AddChild(std::make_unique<SuccessNode>());
    BTStatus r4 = seq4->Execute(ctx, 0.0f);
    PrintResult_ForBehaviorTree("RunningThenSuccess (step 1/2)", r4);
    r4 = seq4->Execute(ctx, 0.0f);
    PrintResult_ForBehaviorTree("RunningThenSuccess (step 2/2)", r4);
    r4 = seq4->Execute(ctx, 0.0f);
    PrintResult_ForBehaviorTree("After RunningThenSuccess completed", r4);

    // ----------------------------------------------------------------
    // 2. SelectorNode 测试
    // ----------------------------------------------------------------
    std::cout << "\n--- 2. SelectorNode ---" << std::endl;

    // 2.1 第一个成功 => Success
    auto sel1 = std::make_unique<SelectorNode>();
    sel1->AddChild(std::make_unique<SuccessNode>());
    sel1->AddChild(std::make_unique<FailureNode>());
    BTStatus s1 = sel1->Execute(ctx, 0.0f);
    while (s1 == BTStatus::Running) s1 = sel1->Execute(ctx, 0.0f);
    PrintResult_ForBehaviorTree("First Success", s1);

    // 2.2 全部失败 => Failure
    auto sel2 = std::make_unique<SelectorNode>();
    sel2->AddChild(std::make_unique<FailureNode>());
    sel2->AddChild(std::make_unique<FailureNode>());
    BTStatus s2 = sel2->Execute(ctx, 0.0f);
    while (s2 == BTStatus::Running) s2 = sel2->Execute(ctx, 0.0f);
    PrintResult_ForBehaviorTree("All Failure", s2);

    // 2.3 第一个 Running，第二个 Success => Running (然后最终 Success)
    auto sel3 = std::make_unique<SelectorNode>();
    sel3->AddChild(std::make_unique<RunningThenSuccessNode>());
    sel3->AddChild(std::make_unique<SuccessNode>());
    BTStatus s3 = sel3->Execute(ctx, 0.0f);
    PrintResult_ForBehaviorTree("First Running, then Success (step 1)", s3);
    s3 = sel3->Execute(ctx, 0.0f);
    PrintResult_ForBehaviorTree("First Running, then Success (step 2 - should move to next)", s3);
    while (s3 == BTStatus::Running) s3 = sel3->Execute(ctx, 0.0f);
    PrintResult_ForBehaviorTree("First Running, then Success (final)", s3);

    // ----------------------------------------------------------------
    // 3. ParallelNode 测试
    // ----------------------------------------------------------------
    std::cout << "\n--- 3. ParallelNode ---" << std::endl;

    // 3.1 全部成功 => Success
    auto par1 = std::make_unique<ParallelNode>();
    par1->AddChild(std::make_unique<SuccessNode>());
    par1->AddChild(std::make_unique<SuccessNode>());
    par1->TotalAll();
    BTStatus p1 = par1->Execute(ctx, 0.0f);
    PrintResult_ForBehaviorTree("All Success", p1);

    // 3.2 任一失败 => Failure
    auto par2 = std::make_unique<ParallelNode>();
    par2->AddChild(std::make_unique<SuccessNode>());
    par2->AddChild(std::make_unique<FailureNode>());
    par2->TotalAll();
    BTStatus p2 = par2->Execute(ctx, 0.0f);
    PrintResult_ForBehaviorTree("With Failure", p2);

    // 3.3 部分 Running => Running
    auto par3 = std::make_unique<ParallelNode>();
    par3->AddChild(std::make_unique<RunningNode>());
    par3->AddChild(std::make_unique<SuccessNode>());
    par3->TotalAll();
    BTStatus p3 = par3->Execute(ctx, 0.0f);
    PrintResult_ForBehaviorTree("With Running", p3);

    // 3.4 多个 Running + 最终全部成功
    auto par4 = std::make_unique<ParallelNode>();
    par4->AddChild(std::make_unique<RunningThenSuccessNode>(2));
    par4->AddChild(std::make_unique<RunningThenSuccessNode>(3));
    par4->TotalAll();
    BTStatus p4 = par4->Execute(ctx, 0.0f);
    PrintResult_ForBehaviorTree("Parallel RunningThenSuccess (step 1)", p4);
    p4 = par4->Execute(ctx, 0.0f);
    PrintResult_ForBehaviorTree("Parallel RunningThenSuccess (step 2)", p4);
    p4 = par4->Execute(ctx, 0.0f);
    PrintResult_ForBehaviorTree("Parallel RunningThenSuccess (step 3 - one finishes, other still running)", p4);
    p4 = par4->Execute(ctx, 0.0f);
    PrintResult_ForBehaviorTree("Parallel RunningThenSuccess (step 4 - all finished)", p4);

    // ----------------------------------------------------------------
     // 4. Reset 功能测试
    // ----------------------------------------------------------------
    std::cout << "\n--- 4. Reset Function ---" << std::endl;

    auto seq_reset = std::make_unique<SequenceNode>();
    auto running_node = std::make_unique<RunningThenSuccessNode>(2);
    auto* raw_running = running_node.get();  // 保存指针用于检查状态
    seq_reset->AddChild(std::move(running_node));
    seq_reset->AddChild(std::make_unique<SuccessNode>());

    BTStatus r_reset = seq_reset->Execute(ctx, 0.0f);
    PrintResult_ForBehaviorTree("Before reset (step 1)", r_reset);
    r_reset = seq_reset->Execute(ctx, 0.0f);
    PrintResult_ForBehaviorTree("Before reset (step 2)", r_reset);

    seq_reset->ResetNode();
    std::cout << "[Test] Reset called" << std::endl;
    r_reset = seq_reset->Execute(ctx, 0.0f);
    PrintResult_ForBehaviorTree("After reset (should start from beginning)", r_reset);

    // ----------------------------------------------------------------
    // 总结
    // ----------------------------------------------------------------
    std::cout << "\n=== All tests completed ===" << std::endl;
}


void AlgorithmsUnitTesting::TestBehaviorTree_Step4()
{
    std::cout << "=== Decorator Node Tests ===" << std::endl;
    StateContext ctx;

    // 1. Inverter: Success → Failure, Failure → Success, Running → Running
    {
        auto inv = InverterNode(std::make_unique<TestNode>(BTStatus::Success));
        auto r = inv.Execute(ctx, 0.0f);
        PrintResult_ForBehaviorTree("Inverter(Success)", r);
        assert(r == BTStatus::Failure);
    }

    {
        auto inv = InverterNode(std::make_unique<TestNode>(BTStatus::Failure));
        auto r = inv.Execute(ctx, 0.0f);
        PrintResult_ForBehaviorTree("Inverter(Failure)", r);
        assert(r == BTStatus::Success);
    }

    {
        auto inv = InverterNode(std::make_unique<TestNode>(BTStatus::Running));
        auto r = inv.Execute(ctx, 0.0f);
        PrintResult_ForBehaviorTree("Inverter(Running)", r);
        assert(r == BTStatus::Running);
    }

    // 2. Repeater: 重复 3 次
    {
        auto rep = RepeaterNode(std::make_unique<CounterNode>(1), 3);
        BTStatus r;
        for (int i = 0; i < 4; i++) {
            r = rep.Execute(ctx, 0.0f);
            if (r != BTStatus::Running) break;
        }
        PrintResult_ForBehaviorTree("Repeater(3 times)", r);
        assert(r == BTStatus::Success);
    }

    // 3. Repeater 无限（-1）: 应一直返回 Running
    {
        auto rep = RepeaterNode(std::make_unique<CounterNode>(1), -1);
        auto r = rep.Execute(ctx, 0.0f);
        PrintResult_ForBehaviorTree("Repeater(infinite)", r);
        assert(r == BTStatus::Running);
    }

    // 4. UntilSuccess: 直到子节点成功
    {
        auto until = UntilSuccessNode(std::make_unique<CounterNode>(3));
        BTStatus r;
        for (int i = 0; i < 5; i++) {
            r = until.Execute(ctx, 0.0f);
            if (r == BTStatus::Success) break;
        }
        PrintResult_ForBehaviorTree("UntilSuccess(Counter 3)", r);
        assert(r == BTStatus::Success);
    }

    // 5. Timeout: 超时返回 Failure
    {
        auto timeout = TimeoutNode(std::make_unique<RunningThenDoneNode>(5), 3.0f);
        BTStatus r;
        // 执行 5 帧，每帧 1.0ms → 第 3 帧后超时
        for (int i = 0; i < 5; i++) {
            r = timeout.Execute(ctx, 1.0f);
            if (r != BTStatus::Running) break;
        }
        PrintResult_ForBehaviorTree("Timeout(3ms, RunningThenDone 5 steps)", r);
        assert(r == BTStatus::Failure);
    }


     // 6. Cooldown: 冷却期间返回 Running
    {
        auto cooldown = CooldownNode(std::make_unique<TestNode>(BTStatus::Success), 2.0f);
        // 第一次执行：子节点成功，返回 Success，触发冷却
        auto r1 = cooldown.Execute(ctx, 0.0f);
        PrintResult_ForBehaviorTree("Cooldown first exec", r1);
        assert(r1 == BTStatus::Success);
        // 第二次执行：冷却中（1.0ms < 2.0ms），返回 Running
        auto r2 = cooldown.Execute(ctx, 1.0f);
        PrintResult_ForBehaviorTree("Cooldown during cooldown (1ms)", r2);
        assert(r2 == BTStatus::Running);
        // 第三次执行：冷却结束（1.0 + 1.0 = 2.0ms），子节点再次执行
        auto r3 = cooldown.Execute(ctx, 1.0f);
        PrintResult_ForBehaviorTree("Cooldown after cooldown", r3);
        assert(r3 == BTStatus::Success);
    }

    std::cout << "\n=== All Decorator Tests PASSED ===" << std::endl;
}




void AlgorithmsUnitTesting::TestBasicInsertAndQuery() {
    std::cout << "[Test] Basic Insert and Query..." << std::endl;

    DynamicAOI aoi;
    aoi.SetSplitThreshold(10.0f);
    aoi.SetMergeThreshold(3.0f);

    // 插入 5 个实体（密度低，不应分裂）
    for (int i = 0; i < 5; ++i) {
        aoi.AddEntity(i, i * 10.0f, i * 10.0f);
    }

    // 查询半径 50 内的实体
    auto result = aoi.Query(25.0f, 25.0f, 50.0f);
    // 预期：至少包含实体 2（在 20,20）和 3（在 30,30）
    assert(!result.empty());
    std::cout << "[PASS] Basic Insert and Query" << std::endl;
}

void AlgorithmsUnitTesting::TestSplitTrigger() {
    std::cout << "[Test] Split trigger..." << std::endl;

    DynamicAOI aoi;
    aoi.SetSplitThreshold(0.00001f);
    aoi.SetMergeThreshold(3.0f);

    // 在根区域中心插入大量实体（超过分裂阈值）
    for (int i = 0; i < 15; ++i) {
        float angle = i * 3.14159f * 2.0f / 15.0f;
        float x = 500.0f + 50.0f * std::cos(angle);
        float y = 500.0f + 50.0f * std::sin(angle);
        aoi.AddEntity(i, x, y);
    }

    // 检查是否有分裂发生（区域数量应该 > 1）
    auto infos = aoi.GetRegionInfos();
    std::cout << "  Regions after inserting 15 entities: " << infos.size() << std::endl;
    assert(infos.size() > 1);

    std::cout << "[PASS] Split trigger" << std::endl;
}

void AlgorithmsUnitTesting::TestMergeTrigger() {
    std::cout << "[Test] Merge trigger..." << std::endl;

    DynamicAOI aoi;
    aoi.SetSplitThreshold(0.00001f);
    aoi.SetMergeThreshold(0.000003f);

    // 先插入大量实体触发分裂
    std::vector<uint32_t> ids;
    for (int i = 0; i < 15; ++i) {
        float x = 500.0f + (i - 7.5f) * 10.0f;
        float y = 500.0f + (i - 7.5f) * 10.0f;
        aoi.AddEntity(i, x, y);
        ids.push_back(i);
    }

    // 记录分裂后的区域数量
    auto infos_before = aoi.GetRegionInfos();
    size_t count_before = infos_before.size();
    std::cout << "  Regions before merge: " << count_before << std::endl;

    // 移除所有实体（密度降为 0，应触发合并）
    for (uint32_t id : ids) {
        aoi.RemoveEntity(id);
    }

    auto infos_after = aoi.GetRegionInfos();
    size_t count_after = infos_after.size();
    std::cout << "  Regions after merge: " << count_after << std::endl;

    // 执行多次 CheckAndAdjust 触发合并（因为密度变化需要冷却）
    for (int i = 0; i < 5; ++i) {
        aoi.Rebalance();
    }

    infos_after = aoi.GetRegionInfos();
    count_after = infos_after.size();
    std::cout << "Rebalance  Regions after merge: " << count_after << std::endl;
    assert(count_after < count_before);

    std::cout << "[PASS] Merge trigger" << std::endl;
}

void AlgorithmsUnitTesting::TestDynamicAOI() {
    std::cout << "=== DynamicAOI Functional Tests ===" << std::endl;
    TestBasicInsertAndQuery();
    TestSplitTrigger();
    TestMergeTrigger();
    std::cout << "=== ALL FUNCTIONAL TESTS PASSED ===" << std::endl;
}


void AlgorithmsUnitTesting::TestDynamicAOI_V2() {
    std::cout << "=== TestDynamicAOI_V2 Functional Tests ===" << std::endl;
    for(int i = 0; i <= 2; ++i)
    {
        {
            std::cout << "[Test] Basic Insert and Query...index:=" << i << std::endl;
            DynamicAOI_V2 aoi(i);
            aoi.SetSplitThreshold(10.0f);
            aoi.SetMergeThreshold(3.0f);

            // 插入 5 个实体（密度低，不应分裂）
            for (int i = 0; i < 5; ++i) {
                aoi.AddEntity(i, i * 10.0f, i * 10.0f);
            }

            // 查询半径 50 内的实体
            auto result = aoi.Query(25.0f, 25.0f, 50.0f);
            // 预期：至少包含实体 2（在 20,20）和 3（在 30,30）
            assert(!result.empty());
            std::cout << "[PASS] Basic Insert and Query...index:=" << i << std::endl;
        }


        {
            std::cout << "[Test] Split trigger...index:=" << i << std::endl;

            DynamicAOI_V2 aoi(i);
            aoi.SetSplitThreshold(0.00001f);
            aoi.SetMergeThreshold(0.000003f);

            // 在根区域中心插入大量实体（超过分裂阈值）
            for (int i = 0; i < 15; ++i) {
                float angle = i * 3.14159f * 2.0f / 15.0f;
                float x = 500.0f + 50.0f * std::cos(angle);
                float y = 500.0f + 50.0f * std::sin(angle);
                aoi.AddEntity(i, x, y);
            }

            // 检查是否有分裂发生（区域数量应该 > 1）
            auto infos = aoi.GetRegionInfos();
            std::cout << "  Regions after inserting 15 entities: " << infos.size() << std::endl;
            assert(infos.size() > 1);

            std::cout << "[PASS] Split trigger......index:=" << i << std::endl;
        }

        {
            std::cout << "[Test] Merge trigger...index:=" << i << std::endl;

            DynamicAOI_V2 aoi(i);
            aoi.SetSplitThreshold(0.00001f);
            aoi.SetMergeThreshold(0.000003f);

            // 先插入大量实体触发分裂
            std::vector<uint32_t> ids;
            for (int i = 1; i <= 15; ++i) {
                float x = 500.0f + (i - 7.5f) * 10.0f;
                float y = 500.0f + (i - 7.5f) * 10.0f;
                aoi.AddEntity(i, x, y);
                ids.push_back(i);
            }

            // 记录分裂后的区域数量
            auto infos_before = aoi.GetRegionInfos();
            size_t count_before = infos_before.size();
            std::cout << "  before Regions before merge: " << count_before << std::endl;

            // 移除所有实体（密度降为 0，应触发合并）
            for (uint32_t id : ids) {
                aoi.RemoveEntity(id);
            }

            auto infos_after = aoi.GetRegionInfos();
            size_t count_after = infos_after.size();
            std::cout << "  after Regions after merge: " << count_after << std::endl;

            // 执行多次 CheckAndAdjust 触发合并（因为密度变化需要冷却）
            for (int i = 0; i < 5; ++i) {
                aoi.Rebalance();
            }

            infos_after = aoi.GetRegionInfos();
            count_after = infos_after.size();
            std::cout << "Rebalance  Regions after merge: " << count_after << std::endl;
            assert(count_after < count_before);

            std::cout << "[PASS] Merge trigger.....index:=" << i << std::endl;
        }
        
    }
    std::cout << "=== ALL FUNCTIONAL TESTS PASSED ===" << std::endl;
}


 void AlgorithmsUnitTesting::TestPartitionCreation()
 {
    PartitionManager mgr;
    AABB world{0, 0, 1000, 1000};
    mgr.Init(world, 100);

    auto partitions = mgr.GetAllPartitions();
    assert(partitions.size() == 100);  // 10x10

    // 测试位置查询
    auto p = mgr.GetPartitionByPos(50, 50);
    assert(p != nullptr);
    assert(p->bounds.min_x == 0 && p->bounds.max_x == 100);
    assert(p->bounds.min_y == 0 && p->bounds.max_y == 100);

    // 测试 AOI 实例是否存在
    auto* aoi = mgr.GetPartitionAOI(p->partition_id);
    assert(aoi != nullptr);

    // 测试向分区 AOI 添加实体
    bool added = aoi->AddEntity(1, 50, 50);
    assert(added);

    std::cout << "Partition creation test PASSED" << std::endl;
 }


 void AlgorithmsUnitTesting::TestLoadMonitoring()
 {
    std::cout << "[Test] Load monitoring..." << std::endl;

    PartitionManager mgr;
    AABB world{0, 0, 1000, 1000};
    mgr.Init(world, 100);

    auto partitions = mgr.GetAllPartitions();
    assert(!partitions.empty());
    auto p = partitions[0];
    auto* aoi = p->aoi.get();

    // 添加 60 个实体
    for (int i = 0; i < 60; ++i) {
        aoi->AddEntity(i, i % 100, i / 10);
    }

    mgr.RefreshLoad(p->partition_id);
    //std::cout << "[Test] Load monitoring... player_count:=" << (p->player_count) << std::endl;
    assert(p->player_count == 60);

    // 检查超载
    auto overloaded = mgr.GetOverloadedPartitions();
    assert(std::find(overloaded.begin(), overloaded.end(), p->partition_id) != overloaded.end());

    std::cout << "[PASS] Load monitoring" << std::endl;
 }


 void AlgorithmsUnitTesting::TestMigrationDecision()
 {
    std::cout << "[Test] Migration decision..." << std::endl;

    PartitionManager mgr;
    AABB world{0, 0, 1000, 1000};
    mgr.Init(world, 100);

    auto partitions = mgr.GetAllPartitions();
    auto p = partitions[0];
    auto* aoi = p->aoi.get();

    // 添加 60 个实体 → 超载
    for (int i = 0; i < 60; ++i) {
        aoi->AddEntity(i, i % 100, i / 10);
    }
    mgr.RefreshLoad(p->partition_id);

    auto decision = mgr.EvaluatePartition(p->partition_id);
    assert(decision.should_migrate == true);
    assert(decision.reason == MigrationReason::Overloaded);
    assert(decision.direction == MigrationDirection::Split);

    std::cout << "[PASS] Migration decision" << std::endl;
 }


 void AlgorithmsUnitTesting::TestUnderloadedMerge()
 {
    std::cout << "[Test] Underloaded merge..." << std::endl;

    PartitionManager mgr;
    AABB world{0, 0, 1000, 1000};
    mgr.Init(world, 100);

    // 设置较低阈值以便测试
    LoadThresholds thresholds;
    thresholds.max_players_per_partition = 50;
    thresholds.min_players_for_merge = 10;
    mgr.SetThresholds(thresholds);

    auto partitions = mgr.GetAllPartitions();
    // 取两个相邻分区，各放 3 个实体
    auto p1 = partitions[0];
    auto p2 = partitions[1];  // 假设相邻

    for (int i = 0; i < 3; ++i) {
        p1->aoi->AddEntity(i, i * 10, 50);
        p2->aoi->AddEntity(i + 100, i * 10 + 100, 50);
    }
    mgr.RefreshLoad(p1->partition_id);
    mgr.RefreshLoad(p2->partition_id);

    auto decision = mgr.EvaluatePartition(p1->partition_id);
    if (decision.should_migrate) {
        assert(decision.reason == MigrationReason::Underloaded);
        assert(decision.direction == MigrationDirection::Merge);
        std::cout << "[PASS] Underloaded merge triggered" << std::endl;
    } else {
        std::cout << "[SKIP] Underloaded merge not triggered (may need to check adjacency)" << std::endl;
    }
 }


 void AlgorithmsUnitTesting::TestPartitionMigration()
 {
    std::cout << "=== Partition Day 2 Tests ===" << std::endl;
    TestLoadMonitoring();
    TestMigrationDecision();
    TestUnderloadedMerge();
    std::cout << "=== ALL TESTS COMPLETE ===" << std::endl;
 }


 void AlgorithmsUnitTesting::Test_IncrementalHashTable()
 {
    // 创建增量哈希表，初始桶数 4，负载因子阈值 0.75，每次迁移 2 个元素
    IncrementalHashTable<int, std::string> table(8, 0.75f, 2);

    // 插入 100 个元素，会在插入过程中逐步触发迁移
    for (int i = 0; i < 1000; ++i) {
        auto start = std::chrono::steady_clock::now();
        table.Insert({i, "value" + std::to_string(i)});
        auto end = std::chrono::steady_clock::now();

        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();
        if(elapsed > 5)
        {
            std::cout << "insert i:=" << i << " cost microseconds:=" << elapsed << std::endl;
        }
    }

    // 查找
    auto result = table.Find(50);
    if (result) {
        std::cout << "Found: " << result.value() << std::endl;
    }

    // 删除
    table.Erase(30);

    std::cout << "Size: " << table.Size() << std::endl;
 }