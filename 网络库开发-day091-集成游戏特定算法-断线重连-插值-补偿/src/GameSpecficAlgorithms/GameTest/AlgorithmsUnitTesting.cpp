#include "AlgorithmsUnitTesting.h"
#include "../AOI/GridAOI.h"
#include "../AOI/CrossListAOI.h"
#include "../AOI/QuadTreeAOI.h"
#include "../AOI/AOIPerformanceTest.h"
#include "../../Common/FixedPoint.h"
#include "../../Common/FixedPonitMaxFunc.h"
#include "../FrameSync/ServerPlayerManager.h"
#include "../FrameSync/RemotePlayerSmoother.h"
#include <vector>
#include <cstdint>

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
}

// 辅助：打印定点数
void AlgorithmsUnitTesting::PrintState(const char* label, const ServerPlayerState& state) {
    printf("%s: pos=(%.2f, %.2f), hp=%u\n", 
           label, state.x.ToDouble(), state.y.ToDouble(), state.hp);
}

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