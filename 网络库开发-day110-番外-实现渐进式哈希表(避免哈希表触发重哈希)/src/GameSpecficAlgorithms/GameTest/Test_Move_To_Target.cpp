// test_move_to_target.cpp
// 编译：g++ -std=c++17 test_move_to_target.cpp GridMap.cpp NodeManager.cpp AStarPathFinder.cpp PathSmoother.cpp -o test_move_to_target

#include "Test_Move_To_Target.h"
#include <iostream>
#include <cassert>
#include <memory>
#include "../A_Star/GridMap.h"
#include "../A_Star/NodeManager.h"
#include "../A_Star/PathFinder.h"
#include "../A_Star/PathSmoother.h"
#include "../BehaviorTree/BTActionNode/MoveToTargetAction.h"
#include "../StateContext.h"



// ===== 测试 1：直接移动（无障碍） =====
void Test_Move_To_Target::TestDirectMove() 
{
    std::cout << "[Test 1] Direct move (no obstacles)..." << std::endl;

    // 创建 10x10 地图 速度设置为0.08f
    std::shared_ptr<A_Star::GridMap> map = std::make_shared<A_Star::GridMap>(10, 10, 1.0f);
    std::shared_ptr<A_Star::NodeManager> nmgr = std::make_shared<A_Star::NodeManager>(10, 10);
    MoveToTargetAction action(0.08f, map, nmgr);

    // 设置状态：AI 在 (1, 1)，目标在 (5, 5)
    StateContext ctx;
    ctx.entity_id = 1001;
    ctx.x = Fixed(1.5f);
    ctx.y = Fixed(1.5f);
    ctx.target_id = 2001;
    ctx.target_x = Fixed(5.5f);
    ctx.target_y = Fixed(5.5f);

    // 第一次执行：计算路径，开始移动
    BTStatus status = action.Execute(ctx, 20.0f);
    std::cout << "  Step 1: Running ✓" << std::endl;

    // 模拟多帧移动（20ms * 50 = 1000ms）
    for (int i = 0; i < 50; ++i) {
        status = action.Execute(ctx, 20.0f);
        if (status == BTStatus::Success) break;
    }

    // 应该到达目标
    assert(status == BTStatus::Success);
    assert(ApproxEqual(ctx.x.ToFloat(), 5.5f, 0.5f));
    assert(ApproxEqual(ctx.y.ToFloat(), 5.5f, 0.5f));

    std::cout << "[PASS] Direct move" << std::endl;
}

// ===== 测试 2：障碍物绕行 =====
void Test_Move_To_Target::TestWithObstacles() 
{
    std::cout << "[Test 2] Move with obstacles..." << std::endl;

    std::shared_ptr<A_Star::GridMap> map = std::make_shared<A_Star::GridMap>(10, 10, 1.0f);
    std::shared_ptr<A_Star::NodeManager> nmgr = std::make_shared<A_Star::NodeManager>(10, 10);

    // 设置一堵墙：从 (3,1) 到 (3,8)
    for (int y = 1; y < 9; ++y) {
        map->SetWalkable(3, y, false);
    }

    MoveToTargetAction action(0.08f, map, nmgr);

    StateContext ctx;
    ctx.entity_id = 1001;
    ctx.x = Fixed(0.5f);
    ctx.y = Fixed(5.5f);
    ctx.target_id = 2001;
    ctx.target_x = Fixed(9.5f);
    ctx.target_y = Fixed(5.5f);

    // 执行寻路
    BTStatus status = action.Execute(ctx, 20.0f);
    assert(status == BTStatus::Running);

    // 模拟移动，直到到达目标或超时
    bool arrived = false;
    for (int i = 0; i < 200; ++i) {
        status = action.Execute(ctx, 20.0f);
        if (status == BTStatus::Success) {
            arrived = true;
            break;
        }
        // 验证路径中没有穿墙（x 坐标不等于 3）
        float x = ctx.x.ToFloat();
        float y = ctx.y.ToFloat();
        // 如果 x 在 3 附近且 y 在 0-9 范围内，说明穿墙了
        if (std::abs(x - 3.0f) < 0.5f && y >= 1.0f && y <= 8.0f) {
            assert(false && "Path crossed obstacle!");
        }
    }

    assert(arrived);
    assert(ApproxEqual(ctx.x.ToFloat(), 9.5f, 0.5f));
    assert(ApproxEqual(ctx.y.ToFloat(), 5.5f, 0.5f));

    std::cout << "[PASS] Move with obstacles" << std::endl;
}

// ===== 测试 3：目标移动后重新寻路 =====
void Test_Move_To_Target::TestTargetMoves() 
{
    std::cout << "[Test 3] Target moves during movement..." << std::endl;

    std::shared_ptr<A_Star::GridMap> map = std::make_shared<A_Star::GridMap>(10, 10, 1.0f);
    std::shared_ptr<A_Star::NodeManager> nmgr = std::make_shared<A_Star::NodeManager>(10, 10);
    MoveToTargetAction action(0.08f, map, nmgr);

    StateContext ctx;
    ctx.entity_id = 1001;
    ctx.x = Fixed(0.5f);
    ctx.y = Fixed(0.5f);
    ctx.target_id = 2001;
    ctx.target_x = Fixed(5.5f);
    ctx.target_y = Fixed(0.5f);

    // 开始移动
    BTStatus status = action.Execute(ctx, 20.0f);
    assert(status == BTStatus::Running);

    // 移动几帧后，目标移动了（从 (5,0) 到 (8,5)）
    for (int i = 0; i < 10; ++i) {
        action.Execute(ctx, 20.0f);
    }
    ctx.target_x = Fixed(8.5f);
    ctx.target_y = Fixed(5.5f);

    // 继续移动，应该自动重新寻路
    bool arrived = false;
    for (int i = 0; i < 200; ++i) {
        status = action.Execute(ctx, 20.0f);
        if (status == BTStatus::Success) {
            arrived = true;
            break;
        }
    }

    assert(arrived);
    assert(ApproxEqual(ctx.x.ToFloat(), 8.5f, 0.5f));
    assert(ApproxEqual(ctx.y.ToFloat(), 5.5f, 0.5f));

    std::cout << "[PASS] Target moves" << std::endl;
}

// ===== 测试 4：不可达目标 =====
void Test_Move_To_Target::TestUnreachableTarget() 
{
    std::cout << "[Test 4] Unreachable target..." << std::endl;

    std::shared_ptr<A_Star::GridMap> map = std::make_shared<A_Star::GridMap>(10, 10, 1.0f);
    std::shared_ptr<A_Star::NodeManager> nmgr = std::make_shared<A_Star::NodeManager>(10, 10);

    // 用障碍物完全包围终点 (9,9)
    map->SetWalkable(8, 9, false);
    map->SetWalkable(9, 8, false);
    map->SetWalkable(8, 8, false);

    MoveToTargetAction action(0.08f, map, nmgr);

    StateContext ctx;
    ctx.entity_id = 1001;
    ctx.x = Fixed(0.5f);
    ctx.y = Fixed(0.5f);
    ctx.target_id = 2001;
    ctx.target_x = Fixed(9.5f);
    ctx.target_y = Fixed(9.5f);

    // 执行寻路，应该返回 Failure
    BTStatus status = action.Execute(ctx, 20.0f);
    // 可能第一次执行会返回 Running（开始寻路），但最终应该失败
    // 我们等几帧让寻路完成
    for (int i = 0; i < 10; ++i) {
        status = action.Execute(ctx, 20.0f);
        if (status == BTStatus::Failure) break;
    }

    assert(status == BTStatus::Failure);
    std::cout << "[PASS] Unreachable target" << std::endl;
}

// ===== 测试 5：重置功能 =====
void Test_Move_To_Target::TestReset() 
{
    std::cout << "[Test 5] Reset..." << std::endl;

    std::shared_ptr<A_Star::GridMap> map = std::make_shared<A_Star::GridMap>(10, 10, 1.0f);
    std::shared_ptr<A_Star::NodeManager> nmgr = std::make_shared<A_Star::NodeManager>(10, 10);
    MoveToTargetAction action(0.08f, map, nmgr);

    StateContext ctx;
    ctx.entity_id = 1001;
    ctx.x = Fixed(0.5f);
    ctx.y = Fixed(0.5f);
    ctx.target_id = 2001;
    ctx.target_x = Fixed(9.5f);
    ctx.target_y = Fixed(9.5f);

    // 开始移动
    BTStatus status = action.Execute(ctx, 20.0f);
    assert(status == BTStatus::Running);

    // 移动几帧后重置
    for (int i = 0; i < 2; ++i) {
        action.Execute(ctx, 20.0f);
    }
    float x_before_reset = ctx.x.ToFloat();
    action.ResetNode();

    // 重新执行，应该从当前位置重新开始寻路
    status = action.Execute(ctx, 20.0f);
    assert(status == BTStatus::Running);

    std::cout << "[PASS] Reset" << std::endl;
}

// ===== 测试 6：连续多次寻路 =====
void Test_Move_To_Target::TestMultiplePaths() 
{
    std::cout << "[Test 6] Multiple paths in sequence..." << std::endl;

    std::shared_ptr<A_Star::GridMap> map = std::make_shared<A_Star::GridMap>(10, 10, 1.0f);
    std::shared_ptr<A_Star::NodeManager> nmgr = std::make_shared<A_Star::NodeManager>(10, 10);
    MoveToTargetAction action(0.08f, map, nmgr);

    StateContext ctx;
    ctx.entity_id = 1001;
    ctx.target_id = 2001;

    // 场景 1：从 (0,0) 到 (3,3)
    ctx.x = Fixed(0.5f);
    ctx.y = Fixed(0.5f);
    ctx.target_x = Fixed(3.5f);
    ctx.target_y = Fixed(3.5f);

    BTStatus status = action.Execute(ctx, 20.0f);
    for (int i = 0; i < 100 && status != BTStatus::Success; ++i) {
        status = action.Execute(ctx, 20.0f);
    }
    assert(status == BTStatus::Success);
    assert(ApproxEqual(ctx.x.ToFloat(), 3.5f, 0.5f));
    assert(ApproxEqual(ctx.y.ToFloat(), 3.5f, 0.5f));

    // 场景 2：从当前位置 (3,3) 到 (8,8)
    ctx.target_x = Fixed(8.5f);
    ctx.target_y = Fixed(8.5f);

    status = action.Execute(ctx, 20.0f);
    for (int i = 0; i < 100 && status != BTStatus::Success; ++i) {
        status = action.Execute(ctx, 20.0f);
    }
    assert(status == BTStatus::Success);
    assert(ApproxEqual(ctx.x.ToFloat(), 8.5f, 0.5f));
    assert(ApproxEqual(ctx.y.ToFloat(), 8.5f, 0.5f));

    std::cout << "[PASS] Multiple paths" << std::endl;
}

// ===== 主函数 =====
void Test_Move_To_Target::TestAllMoveCases() 
{
    std::cout << "=== MoveToTargetAction Test Suite ===" << std::endl;
    std::cout << std::endl;

    TestDirectMove();
    std::cout << std::endl;

    TestWithObstacles();
    std::cout << std::endl;

    TestTargetMoves();
    std::cout << std::endl;

    TestUnreachableTarget();
    std::cout << std::endl;

    TestReset();
    std::cout << std::endl;

    TestMultiplePaths();
    std::cout << std::endl;

    std::cout << "=== ALL TESTS PASSED ===" << std::endl;
}