#pragma once

#include "../State.h"


class AttackState : public State
{
public:
    AttackState();

    virtual ~AttackState();

    // 状态生命周期函数
    virtual void OnEnter(StateContext& ctx) override;
    virtual State* OnUpdate(StateContext& ctx, float delta_ms) override;
    virtual void OnExit(StateContext& ctx) override;

    // 获取状态名称（用于调试日志）
    virtual std::string GetName() const override;

private:
    float attack_cool_down_ = 0.0f;
    float attack_interval_ = 1000.0f; // 1 秒冷却 
};