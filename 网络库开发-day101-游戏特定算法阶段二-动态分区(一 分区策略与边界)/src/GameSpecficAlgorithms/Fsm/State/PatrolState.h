#pragma once

#include "../State.h"
#include <random>
#include <vector>

// （巡逻状态）
class PatrolState : public State
{
public:
    PatrolState();

    virtual ~PatrolState();

    // 状态生命周期函数
    virtual void OnEnter(StateContext& ctx) override;
    virtual State* OnUpdate(StateContext& ctx, float delta_ms) override;
    virtual void OnExit(StateContext& ctx) override;

    // 获取状态名称（用于调试日志）
    virtual std::string GetName() const override;

private:
    // 巡逻点列表
    std::vector<std::pair<Fixed, Fixed>> way_points_;
    size_t current_way_point_ = 0;
    float speed_ = 0.05f; // 单位/毫秒
    float move_timer_ = 0.0f;

    std::mt19937 rng_;
};