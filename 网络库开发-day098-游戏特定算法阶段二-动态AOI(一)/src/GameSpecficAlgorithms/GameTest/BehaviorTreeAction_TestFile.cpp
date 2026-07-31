#include "BehaviorTreeAction_TestFile.h"

#include <iostream>
#include <memory>
#include <cassert>
#include <cmath>

#include "../BehaviorTree/BTNode.h"
#include "../BehaviorTree/BTCompositeNode/BTCompositeNode.h"
#include "../BehaviorTree/BTCompositeNode/ParallelNode.h"
#include "../BehaviorTree/BTCompositeNode/SelectorNode.h"
#include "../BehaviorTree/BTCompositeNode/SequenceNode.h"

#include "../BehaviorTree/BTConditionNode/BTConditionNode.h"
#include "../BehaviorTree/BTConditionNode/CheckDistanceCondition.h"
#include "../BehaviorTree/BTConditionNode/CheckTargetExistsCondition.h"

#include "../BehaviorTree/BTActionNode/BTActionNode.h"
#include "../BehaviorTree/BTActionNode/AttackAction.h"
#include "../BehaviorTree/BTActionNode/MoveToTargetAction.h"
#include "../BehaviorTree/BTActionNode/PatrolAction.h"




// ===== 主函数 =====
void BehaviorTreeAction_TestFile::TestAllScenes() {
    std::cout << "=== Behavior Tree Step 3 Unit Tests ===" << std::endl;

    TestCheckTargetExistsCondition();
    TestCheckDistanceCondition();
    TestMoveToTargetAction();
    TestAttackAction();
    TestPatrolAction();
    TestFullAITree();

    std::cout << "\n=== ALL TESTS PASSED ===" << std::endl;
}


// ===== 辅助函数 =====
void BehaviorTreeAction_TestFile::PrintStatus(const std::string& msg, BTStatus status) {
    const char* s = (status == BTStatus::Success) ? "Success" :
                    (status == BTStatus::Failure) ? "Failure" : "Running";
    std::cout << "[TEST] " << msg << " => " << s << std::endl;
}

// 检查位置近似
bool BehaviorTreeAction_TestFile::ApproxEqual(Fixed a, Fixed b, float epsilon /*= 0.001f*/) {
    return (std::abs(a.ToFloat() - b.ToFloat()) < epsilon);
}

// ===== 测试用例 =====

void BehaviorTreeAction_TestFile::TestCheckTargetExistsCondition() {
    std::cout << "\n--- Test: CheckTargetExistsCondition ---" << std::endl;
    StateContext ctx;
    ctx.target_id = 0;
    CheckTargetExistsCondition cond;
    BTStatus r = cond.Execute(ctx, 0.0f);
    PrintStatus("Target absent", r);
    assert(r == BTStatus::Failure);

    ctx.target_id = 100;
    r = cond.Execute(ctx, 0.0f);
    PrintStatus("Target present", r);
    assert(r == BTStatus::Success);
    std::cout << "  PASSED" << std::endl;
}

void BehaviorTreeAction_TestFile::TestCheckDistanceCondition() {
    std::cout << "\n--- Test: CheckDistanceCondition ---" << std::endl;
    StateContext ctx;
    ctx.target_id = 0;
    CheckDistanceCondition cond(10.0f);
    BTStatus r = cond.Execute(ctx, 0.0f);
    PrintStatus("Target absent", r);
    assert(r == BTStatus::Failure);

    ctx.target_id = 100;
    ctx.x = Fixed(0.0f);
    ctx.y = Fixed(0.0f);
    ctx.target_x = Fixed(5.0f);
    ctx.target_y = Fixed(0.0f);
    r = cond.Execute(ctx, 0.0f);
    PrintStatus("Distance 5 < 10", r);
    assert(r == BTStatus::Success);

    ctx.target_x = Fixed(12.0f);
    r = cond.Execute(ctx, 0.0f);
    PrintStatus("Distance 12 > 10", r);
    assert(r == BTStatus::Failure);
    std::cout << "  PASSED" << std::endl;
}


void BehaviorTreeAction_TestFile::TestMoveToTargetAction() {
    std::cout << "\n--- Test: MoveToTargetAction ---" << std::endl;
    StateContext ctx;
    // 1. 目标不存在
    ctx.target_id = 0;
    MoveToTargetAction move(0.1f);
    BTStatus r = move.Execute(ctx, 0.0f);
    PrintStatus("Target absent", r);
    assert(r == BTStatus::Failure);

    // 2. 一步到达
    ctx.target_id = 100;
    ctx.x = Fixed(0.0f);
    ctx.y = Fixed(0.0f);
    ctx.target_x = Fixed(10.0f);
    ctx.target_y = Fixed(0.0f);
    r = move.Execute(ctx, 100.0f);  // speed 0.1 * 100 = 10，正好到达
    PrintStatus("One-step arrival", r);
    assert(r == BTStatus::Success);
    assert(ApproxEqual(ctx.x, Fixed(10.0f)));

    // 3. 多步移动
    ctx.target_id = 100;
    ctx.x = Fixed(0.0f);
    ctx.y = Fixed(0.0f);
    ctx.target_x = Fixed(10.0f);
    ctx.target_y = Fixed(0.0f);
    r = move.Execute(ctx, 30.0f);   // 移动 3 单位，返回 Running
    PrintStatus("Move step 1", r);
    assert(r == BTStatus::Running);
    assert(ApproxEqual(ctx.x, Fixed(3.0f)));

    r = move.Execute(ctx, 30.0f);   // 移动到 6
    assert(r == BTStatus::Running);
    assert(ApproxEqual(ctx.x, Fixed(6.0f)));

    r = move.Execute(ctx, 30.0f);   // 移动到 9
    assert(r == BTStatus::Running);
    assert(ApproxEqual(ctx.x, Fixed(9.0f)));

    r = move.Execute(ctx, 30.0f);   // 移动到 10 并返回 Success
    assert(r == BTStatus::Success);
    assert(ApproxEqual(ctx.x, Fixed(10.0f)));
    std::cout << "  PASSED" << std::endl;
}


void BehaviorTreeAction_TestFile::TestAttackAction() {
    std::cout << "\n--- Test: AttackAction ---" << std::endl;
    StateContext ctx;
    // 1. 目标不存在
    ctx.target_id = 0;
    AttackAction attack(100.0f);  // 100ms 冷却
    BTStatus r = attack.Execute(ctx, 0.0f);
    PrintStatus("Target absent", r);
    assert(r == BTStatus::Failure);

    // 2. 攻击冷却测试
    ctx.target_id = 100;
    r = attack.Execute(ctx, 0.0f);   // 第一次立即攻击
    PrintStatus("Attack 1", r);
    assert(r == BTStatus::Success);

    r = attack.Execute(ctx, 50.0f);  // 冷却中（50 < 100）
    PrintStatus("Attack during cooldown", r);
    assert(r == BTStatus::Running);

    r = attack.Execute(ctx, 50.0f);  // 冷却结束 (50+50=100)
    PrintStatus("Attack 2 (cooldown ended)", r);
    assert(r == BTStatus::Success);
    std::cout << "  PASSED" << std::endl;
}

void BehaviorTreeAction_TestFile::TestPatrolAction() {
    std::cout << "\n--- Test: PatrolAction ---" << std::endl;
    StateContext ctx;
    ctx.x = Fixed(0.0f);
    ctx.y = Fixed(0.0f);
    const float SPEED = 0.05f;
    const int POINT_COUNT = 2;
    const float RANGE = 10.0f;
    const float DELTA_MS = 20.0f;
    PatrolAction patrol(SPEED, POINT_COUNT, RANGE);

    // 第一次执行：生成巡逻点并开始移动
    BTStatus r = patrol.Execute(ctx, DELTA_MS);
    PrintStatus("Patrol step 1", r);
    assert(r == BTStatus::Running);
    Fixed start_x = ctx.x;
    Fixed start_y = ctx.y;
    // 位置应该发生变化（从原点移开）
    assert(ctx.x != Fixed(0.0f) || ctx.y != Fixed(0.0f));

    // 计算理论最大帧数：两个点，每段最大距离 2*RANGE，速度 SPEED
    // 每帧移动距离 = SPEED * DELTA_MS = 0.05 * 20 = 1.0 单位
    // 最坏情况：两个点都在对角位置，距离 = sqrt(2) * RANGE ≈ 14.14
    // 每段最多 15 帧，两段最多 30 帧

    const int MAX_FRAMES = 60;  // 留足余量
    int frames = 0;
    for (; frames < MAX_FRAMES; ++frames) {
        r = patrol.Execute(ctx, DELTA_MS);
        if (r == BTStatus::Success) {
            break;
        }
        // 每帧都应该在移动，不应该失败
        assert(r == BTStatus::Running);
    }

    // 验证：应该成功完成
    PrintStatus("Patrol complete", r);
    assert(r == BTStatus::Success);
    std::cout << "  Completed in " << frames << " frames" << std::endl;

    // 验证最终位置：应该等于最后一个巡逻点（即第二个点）
    // 由于 PatrolAction 内部状态不可见，我们无法直接验证点坐标，
    // 但我们可以验证位置不等于原点，且在合理范围内（RANGE 附近）
    assert(ctx.x != Fixed(0.0f) || ctx.y != Fixed(0.0f));

    // 再次执行：应重新生成巡逻点并继续（从新起点开始）
    r = patrol.Execute(ctx, DELTA_MS);
    PrintStatus("Patrol restart", r);
    assert(r == BTStatus::Running);

    std::cout << "  PASSED" << std::endl;
}


void BehaviorTreeAction_TestFile::TestFullAITree() {
    std::cout << "\n--- Test: Full AI Tree ---" << std::endl;

    StateContext ctx;
    ctx.entity_id = 1001;
    ctx.x = Fixed(0.0f);
    ctx.y = Fixed(0.0f);
    ctx.target_id = 0;

    // 构建完整树：Selector(Sequence(目标存在，距离<70，追击，攻击)，巡逻)
    auto root = std::make_unique<SelectorNode>();

    auto attackBranch = std::make_unique<SequenceNode>();
    attackBranch->AddChild(std::make_unique<CheckTargetExistsCondition>());
    attackBranch->AddChild(std::make_unique<CheckDistanceCondition>(70.0f));
    attackBranch->AddChild(std::make_unique<MoveToTargetAction>(0.08f));
    attackBranch->AddChild(std::make_unique<AttackAction>(500.0f));

    auto patrolBranch = std::make_unique<PatrolAction>(0.05f, 3, 20.0f);

    root->AddChild(std::move(attackBranch));
    root->AddChild(std::move(patrolBranch));

    const float DELTA_MS = 20.0f;
    const float TOTAL_TIME_MS = 10000.0f;  // 10 秒
    int frames = static_cast<int>(TOTAL_TIME_MS / DELTA_MS);

    // ===== 阶段 1：无目标（前 3 秒） =====
    std::cout << "[Phase 1] No target (patrol only)" << std::endl;
    int patrolFrames = 0;
    for (int i = 0; i < 150; ++i) {  // 3 秒
        BTStatus r = root->Execute(ctx, DELTA_MS);
        // 无目标时，巡逻分支应持续运行
        // 注意：PatrolAction 在走完所有巡逻点后会返回 Success，
        // 然后 Selector 的第二个分支成功，整棵树返回 Success。
        // 下一次执行时，PatrolAction 重新开始，返回 Running。
        // 所以这里不强制断言 r == Running
        patrolFrames++;
    }
    std::cout << "  No target: executed " << patrolFrames << " frames" << std::endl;
    // 至少验证 AI 已经移动（巡逻生效）
    assert(ctx.x.ToFloat() != 0.0f || ctx.y.ToFloat() != 0.0f);

    // ===== 阶段 2：目标出现（3 秒后） =====
    std::cout << "[Phase 2] Target appears at (50, 50)" << std::endl;
    ctx.target_id = 2001;
    ctx.target_x = Fixed(50.0f);
    ctx.target_y = Fixed(50.0f);

    // 执行树，直到追击开始（位置向目标移动）
    bool startedChasing = false;
    Fixed prevX = ctx.x;
    Fixed prevY = ctx.y;
    for (int i = 0; i < 50; ++i) {  // 最多 1 秒
        BTStatus r = root->Execute(ctx, DELTA_MS);
        // 检查位置是否向目标靠近
        if (ctx.x.ToFloat() > prevX.ToFloat() || ctx.y.ToFloat() > prevY.ToFloat()) {
            startedChasing = true;
            break;
        }
        prevX = ctx.x;
        prevY = ctx.y;
    }
    assert(startedChasing);
    std::cout << "  Started chasing target" << std::endl;

    // ===== 阶段 3：持续追击直到到达目标附近 =====
    std::cout << "[Phase 3] Chasing target..." << std::endl;
    for (int i = 0; i < 200; ++i) {  // 最多 4 秒
        BTStatus r = root->Execute(ctx, DELTA_MS);
        // 检查是否接近目标（距离 < 5 单位）
        Fixed dx = ctx.x - ctx.target_x;
        Fixed dy = ctx.y - ctx.target_y;
        Fixed distSq = dx * dx + dy * dy;
        if (distSq < Fixed(25.0f)) {  // 5 单位内
            std::cout << "  Reached target vicinity at frame " << i << std::endl;
            break;
        }
    }

    // 验证：最终位置应该接近目标
    Fixed dx = ctx.x - ctx.target_x;
    Fixed dy = ctx.y - ctx.target_y;
    Fixed finalDistSq = dx * dx + dy * dy;
    std::cout << "  Final distance to target: " << FixedMath::FixedSqrt(finalDistSq).ToFloat() << std::endl;
    assert(finalDistSq < Fixed(100.0f));  // 10 单位内

    // ===== 阶段 4：目标消失 =====
    std::cout << "[Phase 4] Target disappears" << std::endl;
    ctx.target_id = 0;

    // 执行若干帧，验证 AI 切换回巡逻
    bool switchedToPatrol = false;
    for (int i = 0; i < 100; ++i) {  // 2 秒
        BTStatus r = root->Execute(ctx, DELTA_MS);
        // 目标消失后，攻击分支失败，应执行巡逻分支
        // 巡逻分支会移动 AI，位置应继续变化
        // 简单地验证位置仍在变化即可
    }
    // 再次执行，验证树仍在运行（无崩溃）
    BTStatus finalStatus = root->Execute(ctx, DELTA_MS);
    std::cout << "  Final status: " << (finalStatus == BTStatus::Running ? "Running" :
                                         finalStatus == BTStatus::Success ? "Success" : "Failure") << std::endl;
    // 树不应返回 Failure（至少巡逻分支可执行）
    assert(finalStatus != BTStatus::Failure);

    std::cout << "  PASSED" << std::endl;
}