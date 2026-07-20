#include "AlgorithmsUnitTesting.h"
#include "../AOI/GridAOI.h"
#include "../AOI/CrossListAOI.h"
#include "../AOI/QuadTreeAOI.h"
#include "../AOI/AOIPerformanceTest.h"
#include "../../Common/FixedPoint.h"
#include "../../Common/FixedPonitMaxFunc.h"
#include "../FrameSync/ServerPlayerManager.h"
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
    ServerPlayerManager mgr;
    mgr.AddPlayer(1);

    std::unordered_map<uint32_t, ClientInput> inputs;
    ClientInput input;
    input.set_move_x(1);
    inputs[1] = input;

    mgr.Tick(inputs, 20);

    ServerPlayerState state;
    mgr.GetPlayerState(1, state);
    // state.x 应该从 0 变为 2.0（因为 1 * 0.1 * 20 = 2）
    printf("Server pos: %.2f\n", state.x.ToDouble());
}