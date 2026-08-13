#include "MoveToTargetAction.h"
#include <iostream>


MoveToTargetAction::MoveToTargetAction(float speed /*= 0.08f*/,  
    std::shared_ptr<A_Star::GridMap> map /*= nullptr*/, 
    std::shared_ptr<A_Star::NodeManager> node_mgr /*= nullptr*/)
    
    : BTActionNode()
    , speed_(Fixed(speed))
    , map_(map)
    , node_mgr_(node_mgr)
    , path_finder_(std::make_unique<A_Star::PathFinder>(map, node_mgr))
    , path_smoother_(std::make_unique<A_Star::PathSmoother>(map))
{    

    path_finder_->SetHeuristic(A_Star::HeuristicType::Manhattan);
}


void MoveToTargetAction::ResetNode()  
{
    // 重置结点 只是寻路路径需要重新生成，当前所在位置和目标位置不变

    current_path_.clear();
    current_target_index_ = 0;
}


BTStatus MoveToTargetAction::Execute(StateContext& ctx, float delta_ms)  
{
    // 1. 检查目标是否存在
    if(0 == ctx.target_id)
    {
        ResetNode();
        return BTStatus::Failure;
    }

    // 2. 检查是否需要重新计算路径
    bool need_repath = false;
    if(current_path_.empty())
    {
        need_repath = true;
    }
    else
    {
        // 如果目标移动了。
        if(ctx.target_id != last_target_id_)
        {
            need_repath = true;
        }
        else
        {
            float delta_x = std::abs(last_target_x_.ToFloat() - ctx.target_x.ToFloat());
            float delta_y = std::abs(last_target_y_.ToFloat() - ctx.target_y.ToFloat());
            if(delta_x > 1.0f || delta_y > 1.0f)
            {
                need_repath = true;
            }
        }
    }

    // 3. 重新寻路
    if(need_repath)
    {
        // 将当前 AI 位置转换为网格坐标
        std::pair<int32_t, int32_t> start_grid_pos = map_->WorldToGrid(ctx.x.ToFloat(), ctx.y.ToFloat());
        std::cout << "before path find x:=" << ctx.target_x.ToFloat() << " y:=" << ctx.target_y.ToFloat() << std::endl;
        std::pair<int32_t, int32_t> end_grid_pos = map_->WorldToGrid(ctx.target_x.ToFloat(), ctx.target_y.ToFloat());

        // 执行A*算法进行寻路
        A_Star::PathResult path_result_list = path_finder_->FindPath(start_grid_pos.first, start_grid_pos.second, end_grid_pos.first, end_grid_pos.second, allow_diagonal);
        if(false == path_result_list.found || path_result_list.path.size() <= 1)
        {
            std::cout << "path find x:=" << end_grid_pos.first << " y:=" << end_grid_pos.second << std::endl;
            // 不可达
            ResetNode();
            return BTStatus::Failure;
        }

        // 进行路径平滑优化
        size_t path_point_count = path_result_list.path.size();
        std::vector<A_Star::PathPoint> repath_points;
        repath_points.reserve(path_point_count);
        for(int i = 0; i < path_point_count; ++i)
        {
            A_Star::PathPoint point;
            point.grid_x = path_result_list.path[i].first;
            point.grid_y = path_result_list.path[i].second;
            point.world_x = path_result_list.world_path[i].first;
            point.world_y = path_result_list.world_path[i].second;
            repath_points.emplace_back(point);
        }

        current_path_ = path_smoother_->SmoothPath(repath_points);
        current_target_index_ = 0;

        last_target_id_ = ctx.target_id;
        last_target_x_ = ctx.target_x;
        last_target_y_ = ctx.target_y;
    }

    // 暂时关闭进行和阈值比较，如果有需要再进行开启相关代码
    // 5. 比较当前点和目标点的差距
    float delta_x = ctx.x.ToFloat() - ctx.target_x.ToFloat();
    float delta_y = ctx.y.ToFloat() - ctx.target_y.ToFloat();
    float square_dist = delta_x * delta_x + delta_y * delta_y;
    if(square_dist <= (arrival_threshold_*arrival_threshold_))
    {
        ctx.x = ctx.target_x;
        ctx.y = ctx.target_y;
        ResetNode();
        return BTStatus::Success;
    }
    

    // 6.0 正常往前一步
    const A_Star::PathPoint& pre_point = current_path_[current_target_index_];

    int32_t next_target_index = current_target_index_ + 1;
    const A_Star::PathPoint& next_point = current_path_[next_target_index];
    
    

    // 计算走的方向
    // 通过两点计算x 和 y的方向值
    Fixed orignal_dir_x = this->CalcuDirForGridPos(pre_point.grid_x, next_point.grid_x);
    Fixed orignal_dir_y = this->CalcuDirForGridPos(pre_point.grid_y, next_point.grid_y);

    // 如果已经走到了或者越过了，则不再走了，则这个方向值停留在原地
    Fixed dir_x = this->GetDirFromCurrentToNextWorldPos(ctx.x.ToFloat(), next_point.world_x, orignal_dir_x);
    Fixed dir_y = this->GetDirFromCurrentToNextWorldPos(ctx.y.ToFloat(), next_point.world_y, orignal_dir_y);

    // 一个时间段只能走一个方向
    if(false == allow_diagonal)
    {
        float offset_x = std::abs(pre_point.world_x - next_point.world_x);
        float offset_y = std::abs(pre_point.world_y - next_point.world_y);

        if(offset_x >= offset_y && dir_x != Fixed::Zero())
        {
            // 优先走x轴，y轴此时不走
            dir_y = Fixed::Zero();
        }
        else if(offset_x < offset_y && dir_y != Fixed::Zero())
        {
            dir_x = Fixed::Zero();
        }
    }
    

    Fixed move_dist = speed_ * Fixed(delta_ms*1.00f); // 假设 speed_ 是 Fixed
    ctx.x += (dir_x * move_dist);
    ctx.y += (dir_y * move_dist);

    // 到达该点或者超过该点
    Fixed target_dir_x = this->GetDirFromCurrentToNextWorldPos(ctx.x.ToFloat(), next_point.world_x, dir_x);
    Fixed target_dir_y = this->GetDirFromCurrentToNextWorldPos(ctx.y.ToFloat(), next_point.world_y, dir_y);

    if(Fixed::Zero() == target_dir_x && Fixed::Zero() == target_dir_y)
    {
        // 越过目标
        current_target_index_ += 1;
        // 已经到达终点. 到达检查（已到达目标点）
        if(current_target_index_ >= current_path_.size() - 1)
        {
          // 已到达最后一个路径点（即目标）
          // 路径点已遍历完，AI 已到达目标位置
          ctx.x = ctx.target_x;
          ctx.y = ctx.target_y;
          ResetNode();
          return BTStatus::Success;
        }


        ctx.x = Fixed(next_point.world_x);
        ctx.y = Fixed(next_point.world_y);
    }

    return BTStatus::Running;
}


// 直线距离
// 核心执行接口
// - ctx: 状态上下文（与 FSM 共用 StateContext）
// - delta_ms: 时间步长（毫秒）
    /*
    BTStatus MoveToTargetAction::Execute(StateContext& ctx, float delta_ms)  
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
        */