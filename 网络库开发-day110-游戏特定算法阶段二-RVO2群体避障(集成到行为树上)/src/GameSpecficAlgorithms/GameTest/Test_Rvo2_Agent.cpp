#include "Test_Rvo2_Agent.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include "../A_RVO2/RVO2_Simulator.h"

 
void Test_Rvo2_Agent::Test_Simulator_Update() 
{
    std::cout << "=== RVO2 Sub-task 1: Agent Movement ===" << std::endl;

    RVO2::Simulator sim;

    // ---------- 测试 1：单个 Agent 移动到目标 ----------
    std::cout << "\n[Test 1] Single agent moves to target..." << std::endl;
    uint32_t a1 = sim.AddAgent({0.0f, 0.0f}, 0.5f);
    sim.SetTarget(a1, {10.0f, 0.0f});

    // 速度 0.1 单位/ms，每帧 20ms → 每帧移动 2 单位
    // 到达 10 单位需要 5 帧
    for (int frame = 0; frame < 5; ++frame) {
        sim.Update(20.0f);
    }

    const auto& agents = sim.GetAgents();
    float x1 = agents[0].position.x;
    float y1 = agents[0].position.y;
    std::cout << "  Agent 1 position: (" << x1 << ", " << y1 << ")" << std::endl;

    // 应该到达 (10, 0) 附近
    assert(std::abs(x1 - 10.0f) < 0.01f);
    assert(std::abs(y1) < 0.01f);
    std::cout << "  PASS: Agent reached target" << std::endl;

    // ---------- 测试 2：多个 Agent 向不同目标移动（无避障，各自到达） ----------
    std::cout << "\n[Test 2] Multiple agents move to different targets..." << std::endl;
    RVO2::Simulator sim2;

    uint32_t b1 = sim2.AddAgent({0.0f, 0.0f}, 0.5f);
    uint32_t b2 = sim2.AddAgent({0.0f, 5.0f}, 0.5f);
    uint32_t b3 = sim2.AddAgent({0.0f, 10.0f}, 0.5f);

    sim2.SetTarget(b1, {10.0f, 0.0f});
    sim2.SetTarget(b2, {10.0f, 5.0f});
    sim2.SetTarget(b3, {10.0f, 10.0f});

    // 模拟足够帧数到达目标
    for (int frame = 0; frame < 10; ++frame) {
        sim2.Update(20.0f);
    }

    const auto& agents2 = sim2.GetAgents();
    for (const auto& agent : agents2) {
        std::cout << "  Agent " << agent.id << " position: (" 
                  << agent.position.x << ", " << agent.position.y << ")" << std::endl;
        // 验证 x 坐标接近 10
        assert(std::abs(agent.position.x - 10.0f) < 0.01f);
        // 验证 y 坐标接近初始 y
        assert(std::abs(agent.position.y - agent.target.y) < 0.01f);
    }
    std::cout << "  PASS: All agents reached their targets" << std::endl;

    // ---------- 测试 3：获取和修改 Agent ----------
    std::cout << "\n[Test 3] GetAgent and modify..." << std::endl;
    RVO2::Simulator sim3;
    uint32_t c1 = sim3.AddAgent({0.0f, 0.0f}, 0.5f);

    RVO2::Agent* agent_ptr = sim3.GetAgent(c1);
    assert(agent_ptr != nullptr);

    // 直接修改
    agent_ptr->position = {5.0f, 5.0f};
    agent_ptr->target = {10.0f, 10.0f};

    const auto& agents3 = sim3.GetAgents();
    assert(std::abs(agents3[0].position.x - 5.0f) < 0.01f);
    assert(std::abs(agents3[0].position.y - 5.0f) < 0.01f);
    std::cout << "  PASS: GetAgent works correctly" << std::endl;

    // ---------- 测试 4：移除 Agent ----------
    std::cout << "\n[Test 4] RemoveAgent..." << std::endl;
    RVO2::Simulator sim4;
    uint32_t d1 = sim4.AddAgent({0.0f, 0.0f}, 0.5f);
    uint32_t d2 = sim4.AddAgent({0.0f, 1.0f}, 0.5f);

    assert(sim4.GetAgents().size() == 2);

    bool removed = sim4.RemoveAgent(d1);
    assert(removed);
    assert(sim4.GetAgents().size() == 1);
    assert(sim4.GetAgents()[0].id == d2);

    std::cout << "  PASS: RemoveAgent works correctly" << std::endl;

    std::cout << "\n=== Sub-task 1 ALL TESTS PASSED ===" << std::endl;
}


void Test_Rvo2_Agent::TestTwoAgentsCrossing() 
{
    std::cout << "\n[Test 1] Two agents crossing (avoid collision)..." << std::endl;

    RVO2::Simulator sim;

    // Agent 1: 从左向右
    uint32_t a1 = sim.AddAgent({0.0f, 0.0f}, 0.5f);
    sim.SetTarget(a1, {10.0f, 0.0f});

    // Agent 2: 从右向左
    uint32_t a2 = sim.AddAgent({10.0f, 5.0f}, 0.5f);
    sim.SetTarget(a2, {0.0f, 5.0f});

    // 模拟 50 帧（20ms/帧）
    for (int frame = 0; frame < 50; ++frame) {
        sim.Update(20.0f);

        const auto& agents = sim.GetAgents();
        // 检查两个 Agent 是否发生重叠
        float dist = std::sqrt(
            std::pow(agents[0].position.x - agents[1].position.x, 2) +
            std::pow(agents[0].position.y - agents[1].position.y, 2)
        );
        float min_dist = agents[0].radius + agents[1].radius;
        assert(dist >= min_dist - 0.01f);  // 不重叠（允许微小误差）
    }

    std::cout << "  PASS: No collision occurred" << std::endl;
}

void Test_Rvo2_Agent::TestThreeAgentsSameTarget() 
{
    std::cout << "\n[Test 2] Three agents converge to same target..." << std::endl;

    RVO2::Simulator sim;

    uint32_t a1 = sim.AddAgent({0.0f, 0.0f}, 0.5f);
    uint32_t a2 = sim.AddAgent({10.0f, 0.0f}, 0.5f);
    uint32_t a3 = sim.AddAgent({5.0f, -10.0f}, 0.5f);

    sim.SetTarget(a1, {5.0f, 0.0f});
    sim.SetTarget(a2, {5.0f, 0.0f});
    sim.SetTarget(a3, {5.0f, 0.0f});

    for (int frame = 0; frame < 80; ++frame) {
        sim.Update(20.0f);

        //std::cout << "start frame_index:=" << frame << std::endl;

        const auto& agents = sim.GetAgents();
        // 检查是否有重叠
        for (size_t i = 0; i < agents.size(); ++i) {
            for (size_t j = i + 1; j < agents.size(); ++j) {
                float dist = std::sqrt(
                    std::pow(agents[i].position.x - agents[j].position.x, 2) +
                    std::pow(agents[i].position.y - agents[j].position.y, 2)
                );
                float min_dist = agents[i].radius + agents[j].radius;
                if(dist < min_dist)
                {
                    std::cout << "i:=" << i << " j:=" << j << "  dist:=" << dist << " min_dist:=" << min_dist << "  frame_index:=" << frame << std::endl;
                }

                assert(dist >= min_dist - 0.01f);
            }
        }
    }

    std::cout << "  PASS: All agents reached target without collision" << std::endl;
}

