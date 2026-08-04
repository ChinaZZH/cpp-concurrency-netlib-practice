#pragma once

#include "BTActionNode.h"
#include <random>
#include <vector>
#include "../../../Common/FixedPoint.h"
#include "../../../Common/FixedPonitMaxFunc.h"

// 在随机巡逻点之间循环移动。
class PatrolAction : public BTActionNode
{
public:
    explicit PatrolAction(float speed = 0.05f, int point_count = 4, float range = 50.0f)
    : BTActionNode()
    , speed_(Fixed(speed))
    , point_count_(point_count)
    , range_(Fixed(range))
    , rng_(std::random_device{}())
    { }


    virtual ~PatrolAction() = default;

    // 核心执行接口
    // - ctx: 状态上下文（与 FSM 共用 StateContext）
    // - delta_ms: 时间步长（毫秒）
    virtual BTStatus Execute(StateContext& ctx, float delta_ms) override 
    {
        // 首次进入或巡逻点为空时生成
        if(way_points_.empty())
        {
            GenerateWayPoints(ctx);
        }

        if(way_points_.empty())
        {
            return BTStatus::Failure;
        }


        Fixed target_x = way_points_[current_index_].first;
        Fixed target_y = way_points_[current_index_].second;
        Fixed sqrtDistance = ctx.DistanceToPointSq(target_x, target_y);
        if(sqrtDistance < Fixed::One())
        {
            current_index_ = (current_index_ + 1) % way_points_.size();
            if(0 == current_index_)
            {
                ResetNode();
                return BTStatus::Success;
            }

            return BTStatus::Running;
        }

        Fixed move_dist = speed_ * Fixed(delta_ms*1.00f);
        Fixed sqrt_move_dist = move_dist * move_dist;
        if(sqrt_move_dist >= sqrtDistance)
        {
            ctx.x = ctx.target_x;
            ctx.y = ctx.target_y;
        }
        else
        {
            // 计算距离（定点数）
            //Fixed dist = FixedMath::FixedSqrt(sqrtDistance);
            Fixed dist = Fixed(std::sqrt(sqrtDistance.ToDouble()));

            // 计算归一化方向（定点数除法）
            Fixed norm = Fixed::One() / dist;  // 1.0 / dist
            ctx.x += ((target_x - ctx.x) * norm * move_dist);
            ctx.y += ((target_y - ctx.y) * norm * move_dist);
        }

        return BTStatus::Running;
    }

    virtual void ResetNode() override
    {
        way_points_.clear();
        current_index_ = 0;
    }

    virtual std::string GetName() const override 
    {
        return "Patrol";
    }


private:
    void GenerateWayPoints(StateContext& ctx)
    {
        way_points_.clear();
        way_points_.reserve(point_count_);

        std::uniform_real_distribution<float> dist(-range_.ToFloat(), range_.ToFloat());
        for(int i = 0; i < point_count_; ++i)
        {
            Fixed x = ctx.x + Fixed(dist(rng_));
            Fixed y = ctx.y + Fixed(dist(rng_));
            way_points_.emplace_back(std::pair(x, y));
        }

        current_index_ = 0;
    }

private:
    std::vector<std::pair<Fixed, Fixed>> way_points_;
    size_t current_index_ = 0;
    Fixed speed_;
    int point_count_;
    Fixed range_;
    std::mt19937 rng_;
};