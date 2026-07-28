#include "ChaseState.h"
#include "AttackState.h"
#include "PatrolState.h"
#include "IdleState.h"
#include <iostream>
#include <cmath>

/*
 float speed_ = 0.08f; // 比巡逻快一些 
*/

ChaseState::ChaseState() = default;

ChaseState::~ChaseState() = default;

// 状态生命周期函数
void ChaseState::OnEnter(StateContext& ctx) 
{
    std::cout << "[Chase] AI " << ctx.entity_id << " starts chasing target " << ctx.target_id << std::endl;
}

State* ChaseState::OnUpdate(StateContext& ctx, float delta_ms)
{
     // 如果目标消失，返回待机
    if(0 == ctx.target_id)
    {
        return new IdleState();
    }

    // 检查距离，如果进入攻击范围，切换攻击
    Fixed sqrtDis = ctx.DistanceToTargetSq();
    Fixed sqrtAttackDis(45.0f * 45.0f);
    if(sqrtDis < sqrtAttackDis)  // // 小于50单位 
    {
        return new AttackState();
    }

    // 如果目标太远（丢失目标），返回待机
    Fixed sqrtChaseDis(500.0f * 500.0f); // 
    if(sqrtDis > sqrtChaseDis)
    {
        return new IdleState();
    }

    // 计算距离（定点数）
    Fixed dist = FixedMath::FixedSqrt(sqrtDis);
    if(dist == Fixed::Zero() || dist < Fixed::One()) {
       return new AttackState();
    }

    // 计算归一化方向（定点数除法）
    Fixed norm = Fixed::One() / dist;  // 1.0 / dist

    // 计算本帧移动距离（定点数）
    Fixed move_dist = Fixed(speed_) * Fixed(delta_ms*1.00f); // 假设 speed_ 是 Fixed

    // 更新位置（定点数运算）
    if((norm * move_dist) >= Fixed::One())
    {
        ctx.x = ctx.target_x;
        ctx.y = ctx.target_y;
    }
    else
    {
        ctx.x += ((ctx.target_x - ctx.x) * norm * move_dist);
        ctx.y += ((ctx.target_y - ctx.y) * norm * move_dist);
    }
    
    return nullptr;

}

void ChaseState::OnExit(StateContext& ctx)
{
    std::cout << "[Chase] AI " << ctx.entity_id << " exits chase" << std::endl;
}

// 获取状态名称（用于调试日志）
std::string ChaseState::GetName() const
{
    return "Chase";
}