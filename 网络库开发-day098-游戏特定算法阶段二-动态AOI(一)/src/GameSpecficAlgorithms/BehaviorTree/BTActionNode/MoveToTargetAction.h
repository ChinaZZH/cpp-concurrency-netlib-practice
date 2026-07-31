#pragma once

#include "BTActionNode.h"
#include <cmath> 


// 向目标移动，到达后返回 Success，目标丢失返回 Failure。
class MoveToTargetAction : public BTActionNode
{
public:
    explicit MoveToTargetAction(float speed = 0.08f)
    : BTActionNode()
    , speed_(Fixed(speed))
    { }


    virtual ~MoveToTargetAction() = default;

    // 核心执行接口
    // - ctx: 状态上下文（与 FSM 共用 StateContext）
    // - delta_ms: 时间步长（毫秒）
    virtual BTStatus Execute(StateContext& ctx, float delta_ms) override 
    {
        if(0 == ctx.target_id)
        {
            ResetNode();
            return BTStatus::Failure;
        }

        // 当前到目标的距离
        Fixed sqrtFixDis = ctx.DistanceToTargetSq();

        // 计算本帧移动距离（定点数）
        Fixed move_dist = speed_ * Fixed(delta_ms*1.00f); // 假设 speed_ 是 Fixed
        Fixed sqrt_move_dist = move_dist * move_dist;
        if(sqrt_move_dist >= sqrtFixDis)
        {
            ctx.x = ctx.target_x;
            ctx.y = ctx.target_y;
            ResetNode();
            return BTStatus::Success;
        }


        // 计算距离（定点数）需要精确性，则使用std::sqrt
        //Fixed dist = FixedMath::FixedSqrt(sqrtFixDis);
        Fixed dist = Fixed(std::sqrt(sqrtFixDis.ToDouble()));
        //std::cout << "MoveToTargetAction sqrtFixDis:=" << sqrtFixDis.ToFloat() << " dist:=" << dist.ToFloat() << std::endl;

        // 计算归一化方向（定点数除法）
        Fixed normal = Fixed::One() / dist;

        Fixed dx = (ctx.target_x - ctx.x);
        Fixed dy = (ctx.target_y - ctx.y);
        //std::cout << "MoveToTargetAction dx:=" << dx.ToFloat() << " dy:=" << dy.ToFloat() << " move_dist:=" << move_dist.ToFloat() << std::endl;
        //std::cout << "MoveToTargetAction dx111:=" << ((ctx.target_x - ctx.x) * normal * move_dist).ToFloat() << " dx222:=" << (((ctx.target_x - ctx.x) / dist) * move_dist).ToFloat() << std::endl;

        ctx.x += (ctx.target_x - ctx.x) * normal * move_dist;
        ctx.y += (ctx.target_y - ctx.y) * normal * move_dist;
        return BTStatus::Running;
    }

    virtual std::string GetName() const override 
    {
        return "MoveToTarget";
    }


private:
    Fixed speed_;
};