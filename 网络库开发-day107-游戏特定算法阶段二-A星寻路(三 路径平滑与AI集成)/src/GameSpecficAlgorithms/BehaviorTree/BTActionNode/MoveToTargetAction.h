#pragma once

#include "BTActionNode.h"
#include <cmath> 
#include <memory>
#include "../../A_Star/PathSmoother.h"
#include "../../A_Star/GridMap.h"
#include "../../A_Star/GridNode.h"
#include "../../A_Star/NodeManager.h"
#include "../../A_Star/PathFinder.h"




// 向目标移动，到达后返回 Success，目标丢失返回 Failure。
class MoveToTargetAction : public BTActionNode
{
public:
    explicit MoveToTargetAction(float speed = 0.08f, 
        std::shared_ptr<A_Star::GridMap> map = nullptr, 
        std::shared_ptr<A_Star::NodeManager> node_mgr = nullptr
    );


    virtual ~MoveToTargetAction() = default;

    // 核心执行接口
    // - ctx: 状态上下文（与 FSM 共用 StateContext）
    // - delta_ms: 时间步长（毫秒）
    virtual BTStatus Execute(StateContext& ctx, float delta_ms) override;

    virtual std::string GetName() const override { return "MoveToTarget"; }

    virtual void ResetNode() override;

private:
    Fixed CalcuDirForGridPos(int src_grid_value, int target_grid_value)
    {
        if(src_grid_value == target_grid_value)
        {
            return Fixed::Zero();
        }

        return (target_grid_value > src_grid_value? Fixed::One() : Fixed(-1));
    }

    // 是否还需要前行，前行的方向
    Fixed GetDirFromCurrentToNextWorldPos(float current_world_pos, float next_world_pos, const Fixed& fixDir)
    {
        if(fixDir == Fixed::Zero())
        {
            return Fixed::Zero();
        }

        if(fixDir == Fixed::One() && current_world_pos >= next_world_pos)
        {
           return Fixed::Zero();
        }

        if(fixDir == Fixed(-1) && current_world_pos <= next_world_pos)
        {
           return Fixed::Zero();
        }

        return fixDir;
    }

private:
    Fixed speed_;

    // 寻路相关
    std::shared_ptr<A_Star::GridMap> map_;
    std::shared_ptr<A_Star::NodeManager> node_mgr_;
    std::unique_ptr<A_Star::PathFinder> path_finder_;
    std::unique_ptr<A_Star::PathSmoother> path_smoother_;

    // 当前路径
    std::vector<A_Star::PathPoint> current_path_;
    size_t current_target_index_ = 0;

    // 移动参数
    float arrival_threshold_ = 1.0f;  // 到底阈值(世界单位) 可以设置为5.0f
    bool allow_diagonal = true;

    // 路径缓存
    uint32_t last_target_id_ = 0;
    Fixed last_target_x_;
    Fixed last_target_y_;

};