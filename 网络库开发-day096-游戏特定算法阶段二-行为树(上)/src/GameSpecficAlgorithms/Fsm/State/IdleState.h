#pragma once

#include "../State.h"
#include <random>

// （待机状态）
class IdleState : public State
{
public:
    IdleState();

    virtual ~IdleState();

    // 状态生命周期函数
    virtual void OnEnter(StateContext& ctx) override;
    virtual State* OnUpdate(StateContext& ctx, float delta_ms) override;
    virtual void OnExit(StateContext& ctx) override;

    // 获取状态名称（用于调试日志）
    virtual std::string GetName() const override;

private:
    float idle_timer_ = 0.0f;
    float idle_duration_ = 0.0f;

    std::mt19937 rng_;
    std::uniform_real_distribution<float> dist_duration_{1.0f, 3.0f};
};