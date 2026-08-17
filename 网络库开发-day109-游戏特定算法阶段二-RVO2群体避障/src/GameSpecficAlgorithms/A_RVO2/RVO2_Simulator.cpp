#include "RVO2_Simulator.h"
#include <algorithm>
#include <cstdint>
#include <cmath>

namespace RVO2
{
    // 添加
    uint32_t Simulator::AddAgent(const Point2D& position, float radius /*= 0.5f*/)
    {
        Agent new_agent;
        new_agent.id = next_agent_id_;
        next_agent_id_ += 1;

        new_agent.position = position;
        new_agent.velocity = {0.0f, 0.0f};
        new_agent.target = position;
        new_agent.radius = radius;
        new_agent.max_speed = 0.1f;
        new_agent.max_accel = 0.05f;
        new_agent.enabled = true;

        agents_.push_back(new_agent);
        return new_agent.id;
    }

    // 移除
    bool Simulator::RemoveAgent(uint32_t agent_id)
    {
        size_t removed_num = std::erase_if(agents_, [agent_id](const Agent& agent){
            return agent.id == agent_id;
        });

        return removed_num > 0 ? true : false;
    }


    // 获取单个 Agent（可修改）
    Agent* Simulator::GetAgent(uint32_t agent_id)
    {
        auto itr = std::find_if(agents_.begin(), agents_.end(), [agent_id](const Agent& agent){
            return agent.id == agent_id;
        });

        if(itr == agents_.end())
        {
            return nullptr;
        }

        return &(*itr);
    }


    // 设置 Agent 目标
    void Simulator::SetTarget(uint32_t agent_id, const Point2D& target)
    {
        Agent* pAgent = this->GetAgent(agent_id);
        if(!pAgent)
        {
            return;
        }

        pAgent->target = target;
    }

        // 更新所有的Agent(每帧调用)
    void Simulator::Update(float delta_ms)
    {
        // 子任务 1：只有基础框架，暂时不实现避障算法
        // 所有 Agent 直接向目标移动（无避障）
        for(auto& agent : agents_)
        {
            if(false == agent.enabled)
            {
                continue;
            }

            float dx = agent.target.x - agent.position.x;
            float dy = agent.target.y - agent.position.y;
            float dist = std::sqrt(dx * dx + dy * dy);

            if(dist < 0.01f)
            {
                continue;
            }

            // 向目标移动一步
            float max_step = agent.max_speed * delta_ms;
            float step = std::min(dist, max_step);
            agent.position.x += (dx / dist) * step;
            agent.position.y += (dy / dist) * step;
        }
    }
}