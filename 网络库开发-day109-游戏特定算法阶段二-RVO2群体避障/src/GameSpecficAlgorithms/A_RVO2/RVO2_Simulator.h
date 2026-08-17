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

        // 设置避障参数
        void SetAvoidanceStrength(float strength) { avoidance_strength_ = strength; }

    private:
        std::vector<Point2D> GenerateCandidateVelocities(const Agent& agent, const Point2D& ideal_vel) const;    
    
        bool IsCollision(const Agent& agent, const Point2D& candidates_vel, float delta_ms, const std::vector<Agent>& others) const;    

        Point2D SelectBestVelocity(const Agent& agent, const std::vector<Point2D>& candidates, const Point2D& ideal_ve, float delta_msl) const;

        float Distance(const Point2D& a, const Point2D& b) const;
        
    private:
        std::vector<Agent> agents_;

        uint32_t next_agent_id_ = 1;

        float avoidance_strength_ = 1.0f;   // 避障强度（预留）
    };
}