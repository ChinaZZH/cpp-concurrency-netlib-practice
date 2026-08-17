#include "RVO2_Simulator.h"
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <iostream>

namespace RVO2
{
    // 添加
    uint32_t Simulator::AddAgent(const Point2D& position, float radius /*= 0.5f*/)
    {
        Agent new_agent;
        new_agent.id = next_agent_id_;
        next_agent_id_ += 1;

        new_agent.position = position;
        new_agent.velocity = Point2D(0.0f, 0.0f);
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

            // 1. 计算理想速度
            float dx = agent.target.x - agent.position.x;
            float dy = agent.target.y - agent.position.y;
            float dist = std::sqrt(dx * dx + dy * dy);

            // 已经到达目标，速度为0.
            if(dist < 0.01f)
            {
                agent.velocity = Point2D(0.0f, 0.0f);
                continue;
            }

            float speed = std::min(agent.max_speed, dist / delta_ms);
            float x_velocity = (dx / dist) * speed;
            float y_velocity = (dy / dist) * speed;
            Point2D ideal_vel = Point2D( x_velocity, y_velocity );

            // 2. 生成候选速度
            auto candidates = this->GenerateCandidateVelocities(agent, ideal_vel);

            // 3. 选择最优速度（避障）
            Point2D chosen_vel = this->SelectBestVelocity(agent, candidates, ideal_vel, delta_ms);

            // 4. 应用速度
            agent.velocity = chosen_vel;
            agent.position.x += (chosen_vel.x * delta_ms);
            agent.position.y += (chosen_vel.y * delta_ms);
        }
    }


    // 生成候选速度
    std::vector<Point2D> Simulator::GenerateCandidateVelocities(const Agent& agent, const Point2D& ideal_vel) const
    {
        std::vector<Point2D> candidates;
        
        // 1. 理想速度
        Point2D src(0.0f, 0.0f);
        float ideal_speed = Distance(src, ideal_vel);

        // 2. 如果理想速度接近 0，增加 8 方向候选
        if(ideal_speed < 0.001f)
        {
            const float angles[] = { 0.0f, 45.0f, 90.0f, 135.0f, 180.0f, 225.0f, 270.0f, 315.0f };
            for(float angle : angles)
            {
                float rad = angle * 3.14159265f / 180.0f;
                Point2D angle_point(std::cos(rad)*agent.max_speed,  std::sin(rad)*agent.max_speed);
                candidates.push_back(angle_point);
            }

            return candidates;
        }


        // 3. 左右偏转 15°, 30°, 45°
        float base_angle = std::atan2(ideal_vel.y, ideal_vel.x);
        const float offsets[] = { -60.0f, -45.0f, -30.0f, -20.0f, -15.0f, -10.0f, -5.0f, 5.0f, 10.0f, 15.0f, 20.0f, 30.0f, 45.0f, 60.0f };
        for(float offset : offsets)
        {
            float rad = offset * 3.14159265f / 180.0f;
            float angle = base_angle + rad;
            Point2D angle_point(std::cos(angle)*ideal_speed,  std::sin(angle)*ideal_speed);
            candidates.push_back(angle_point);
        }

        // 4. 减速（速度减半，方向不变）
        Point2D half_point(ideal_vel.x*0.5f, ideal_vel.y*0.5f);
        candidates.push_back(half_point);

        // 5. 停止
        //Point2D stop_point(0.0f, 0.0f);
        //candidates.push_back(stop_point);

        return candidates;
    }
 
    // 碰撞检测
    bool Simulator::IsCollision(
        const Agent& agent, 
        const Point2D& candidates_vel, 
        float delta_ms, 
        const std::vector<Agent>& others) const
    {
    
        // 计算下一步位置
        float next_x = agent.position.x + candidates_vel.x * delta_ms;
        float next_y = agent.position.y + candidates_vel.y * delta_ms;
        Point2D next_pos(next_x, next_y);

        for(const auto& other : others)
        {
            if((other.id == agent.id) || (false == other.enabled))
            {
                continue;
            }

            float dist = Distance(next_pos, other.position);
            float min_dist = agent.radius + other.radius;
            min_dist = min_dist * 1.5f;
            
            // 会发生碰撞
            if(dist < min_dist)
            {
                return true;
            }
        }

        return false;
    }

    Point2D Simulator::SelectBestVelocity(
        const Agent& agent, 
        const std::vector<Point2D>& candidates, 
        const Point2D& ideal_vel,
        float delta_ms) const
    {
        Point2D best_vel = ideal_vel;
        float best_score = -1e9f;

        // 分离安全候选和危险候选
        std::vector<Point2D>  safe_candidates;
        std::vector<Point2D>  unsafe_candidates;

        for(const auto& cand : candidates)
        {
            if(this->IsCollision(agent, cand, delta_ms, agents_))
            {
                unsafe_candidates.push_back(cand);
            }
            else
            {
                safe_candidates.push_back(cand);
            }
        }

        // 没有安全选项的时候，停在那儿不动
        if(safe_candidates.empty())
        {
            //std::cout << "safe_candidates empty" << std::endl;
            return Point2D(0.0f, 0.0f);
        }

        // 优先使用安全候选；如果没有安全候选，则使用危险候选（避免卡死）
        const std::vector<Point2D>& pool = safe_candidates;
        float ideal_speed = Distance(Point2D(0.0f, 0.0f), ideal_vel);
        for(const auto& cand : pool)
        {
            float cand_speed = Distance(Point2D(0.0f, 0.0f), cand);
            
            // 如果 ideal_speed > 0 且 cand_speed == 0，移动比停在原地强
            if (ideal_speed > 0.001f && cand_speed < 0.001f) 
            {
                continue;
            }

            // 评分：方向一致性 + 速度保持
            float dot = ideal_vel.x * cand.x + ideal_vel.y * cand.y;
            float normal_ideal = Distance(Point2D(0.0f, 0.0f), ideal_vel);
            float normal_cand = Distance(Point2D(0.0f, 0.0f), cand);
            
            float cos_angle = 0.0f;
            if(normal_ideal > 0.001f && normal_cand > 0.001f)
            {
                cos_angle = dot / (normal_cand * normal_ideal);
            }

            // 速度变化惩罚
            float speed_change = Distance(agent.velocity, cand);

            // 评分：方向一致 + 速度变化小
            float score = cos_angle*10.0f - speed_change*0.5f;

            // 都是安全选项，则都加跟都没加没啥区别，则注释掉
            // 安全候选加分
            // score += 5.0f;
           
            if(score > best_score)
            {
                best_score = score;
                best_vel = cand;
            }
        }

        return best_vel;
    }

    float Simulator::Distance(const Point2D& a, const Point2D& b) const
    {
        float delta_x = a.x - b.x;
        float delta_y = a.y - b.y;
        return std::sqrt(delta_x * delta_x + delta_y * delta_y);
    }
}