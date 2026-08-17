#pragma once

#include <vector>
#include <cstdint>
#include "Agent.h"

namespace RVO2
{
    class Simulator
    {
    public:
        Simulator() = default;
        ~Simulator() = default;

        // 添加
        uint32_t AddAgent(const Point2D& position, float radius = 0.5f);

        // 移除
        bool RemoveAgent(uint32_t agent_id);

        // 获取所有的Agent(只读)
        const std::vector<Agent>& GetAgents() const { return agents_; }

        // 获取单个 Agent（可修改）
        Agent* GetAgent(uint32_t agent_id);

        // 更新所有的Agent(每帧调用)
        void Update(float delta_ms);

        // 设置 Agent 目标
        void SetTarget(uint32_t agent_id, const Point2D& target);

    private:
        std::vector<Agent> agents_;
        uint32_t next_agent_id_ = 1;
    };
}