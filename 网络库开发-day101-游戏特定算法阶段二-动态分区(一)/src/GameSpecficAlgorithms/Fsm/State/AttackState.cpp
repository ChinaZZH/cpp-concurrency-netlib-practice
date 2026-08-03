#include "AttackState.h"
#include "PatrolState.h"
#include "ChaseState.h"
#include "IdleState.h"
#include <iostream>

/*
float attack_cool_down_ = 0.0f;
float attack_interval_ = 1000.0f; // 1 秒冷却 
*/

AttackState::AttackState() = default;


AttackState::~AttackState() = default;

// 状态生命周期函数
void AttackState::OnEnter(StateContext& ctx) 
{
    attack_cool_down_ = 0.0f;
    std::cout << "[Attack] AI " << ctx.entity_id << " attack target " << ctx.target_id << std::endl;
}

State* AttackState::OnUpdate(StateContext& ctx, float delta_ms)
{
    // 如果目标消失，返回待机
    if(0 == ctx.target_id)
    {
        return new IdleState();
    }

    // 检查距离，如果目标跑远则追击
    Fixed distance = ctx.DistanceToTargetSq();

    float fMaxDistance =  80.0f * 80.0f;
    Fixed maxDistance(fMaxDistance);
    if(distance > maxDistance)  // // 超过 70 单位
    {
        return new ChaseState();
    }

    // 攻击冷却 
    attack_cool_down_ += delta_ms;
    if(attack_cool_down_ >= attack_interval_)
    {
        attack_cool_down_ = 0.0f;
        // 执行一次攻击（减少目标血量）
        // 实际项目中会通过事件或回调通知外部系统
        std::cout << "[Attack] AI " << ctx.entity_id << " deals damage to " << ctx.target_id << std::endl;
    }

    return nullptr;
}

void AttackState::OnExit(StateContext& ctx)
{
    std::cout << "[Attack] AI " << ctx.entity_id << " exits attack" << std::endl;
}

// 获取状态名称（用于调试日志）
std::string AttackState::GetName() const
{
    return "Attack";
}