#include "IdleState.h"
#include "PatrolState.h"
#include "ChaseState.h"
#include <iostream>

IdleState::IdleState()
: rng_(std::random_device{}())
{

}

IdleState::~IdleState() = default;

// 状态生命周期函数
void IdleState::OnEnter(StateContext& ctx) 
{
    idle_duration_ = dist_duration_(rng_);
    idle_timer_ = 0.0f;
    std::cout << "[Idle] AI " << ctx.entity_id << " enters idle for " << idle_duration_ << "s" << std::endl;
}

State* IdleState::OnUpdate(StateContext& ctx, float delta_ms)
{
    idle_timer_ += static_cast<float>(delta_ms) / 1000.00f; // 转换为秒

    // 检查是否遇到敌人（假设目标存在且距离较近）
    if(0 != ctx.target_id)
    {
        Fixed fixDistance = ctx.DistanceToTargetSq();
        Fixed maxDis(10000.0f);  // 100 单位内
        if(fixDistance < maxDis)
        {
            return new ChaseState(); // 直接返回新状态，由状态机切换
        }
    }

    // 空闲时间到，切换到巡逻
    if(idle_timer_ >= idle_duration_)
    {
       return new PatrolState();
    }

    return nullptr;
}

void IdleState::OnExit(StateContext& ctx)
{
    std::cout << "[Idle] AI " << ctx.entity_id << " exits idle" << std::endl;
}

// 获取状态名称（用于调试日志）
std::string IdleState::GetName() const
{
    return "Idle";
}