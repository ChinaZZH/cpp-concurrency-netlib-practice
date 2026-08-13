
#pragma once
#include <cmath>

class Test_Move_To_Target
{
public:
    void TestAllMoveCases();


private:
    // ===== 辅助函数：判断位置是否近似 =====
    bool ApproxEqual(float a, float b, float epsilon = 0.1f) {
        return std::abs(a - b) < epsilon;
    }

    // ===== 测试 1：直接移动（无障碍） =====
    void TestDirectMove();

    // ===== 测试 2：障碍物绕行 =====
    void TestWithObstacles();

    // ===== 测试 3：目标移动后重新寻路 =====
    void TestTargetMoves();

    // ===== 测试 4：不可达目标 =====
    void TestUnreachableTarget();

    // ===== 测试 5：重置功能 =====
    void TestReset();

    // ===== 测试 6：连续多次寻路 =====
    void TestMultiplePaths();
};