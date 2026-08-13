#pragma once

#include "../State.h"


class ChaseState : public State
{
public:
    ChaseState();

    virtual ~ChaseState();

    // 状态生命周期函数
    virtual void OnEnter(StateContext& ctx) override;
    virtual State* OnUpdate(StateContext& ctx, float delta_ms) override;
    virtual void OnExit(StateContext& ctx) override;

    // 获取状态名称（用于调试日志）
    virtual std::string GetName() const override;

private:
    float speed_ = 0.08f; // 比巡逻快一些
};