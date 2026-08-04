#include "PatrolState.h"
#include "ChaseState.h"
#include "AttackState.h"
#include "IdleState.h"
#include <iostream>
#include <cmath>

/*
// 巡逻点列表
    std::vector<std::pair<float, float>> way_points_;
    size_t current_way_point_ = 0;
    float speed_ = 0.05f; // 单位/毫秒
    float move_timer_ = 0.0f;

    std::mt19937 rng_;
*/

PatrolState::PatrolState() 
: rng_(std::random_device{}())
{

}

PatrolState::~PatrolState() = default;

// 状态生命周期函数
void PatrolState::OnEnter(StateContext& ctx) 
{
    // 生成随机巡逻点（相对于当前位置）
    way_points_.clear();
    std::uniform_real_distribution<float> dist_duration{-50.0f, 50.0f};
    for(int i = 0; i < 4; ++i)
    {
        Fixed x = ctx.x + Fixed(dist_duration(rng_));
        Fixed y = ctx.y + Fixed(dist_duration(rng_));
        way_points_.push_back({x, y});
    }

    current_way_point_ = 0;
    move_timer_ = 0.0f;
    std::cout << "[Patrol] AI " << ctx.entity_id << " starts patrol " << std::endl;
}

State* PatrolState::OnUpdate(StateContext& ctx, float delta_ms)
{
     // 检查是否遇到敌人（优先于巡逻）
     if(0 != ctx.target_id)
     {
         Fixed sqrtDistance = ctx.DistanceToTargetSq();
         Fixed sqrtMaxDis(20000.0f); // 约 141 单位
         if(sqrtDistance < sqrtMaxDis)
         {
            return new ChaseState();
         }
     }

     // 巡逻点没有了
     if(way_points_.empty())
     {
        return nullptr;
     }

     // 向当前目标点移动
     Fixed target_x =  way_points_[current_way_point_].first;
     Fixed target_y = way_points_[current_way_point_].second;
     Fixed sqrtDis = ctx.DistanceToPointSq(target_x, target_y);

     // 计算距离（定点数）
     Fixed dist = FixedMath::FixedSqrt(sqrtDis);
     if(dist == Fixed::Zero() || dist < Fixed::One()) {
        // 到达目标点，切换下一个
        current_way_point_ = (current_way_point_ + 1) % way_points_.size();
        return nullptr;
     }

  
    // 计算本帧移动距离（定点数）
    // 更新位置（定点数运算
    Fixed move_dist = Fixed(speed_) * Fixed(delta_ms*1.00f); // 假设 speed_ 是 Fixed
    if(move_dist >= dist)
    {
        ctx.x = target_x;
        ctx.y = target_y;
    }
    else
    {
         // 计算归一化方向（定点数除法）
        Fixed norm = Fixed::One() / dist;  // 1.0 / dist
        ctx.x += ((target_x - ctx.x) * norm * move_dist);
        ctx.y += ((target_y - ctx.y) * norm * move_dist);
    }

    
    return nullptr;
}

void PatrolState::OnExit(StateContext& ctx)
{
    std::cout << "[Patrol] AI " << ctx.entity_id << " exits patrol" << std::endl;
}

// 获取状态名称（用于调试日志）
std::string PatrolState::GetName() const
{
    return "Patrol";
}