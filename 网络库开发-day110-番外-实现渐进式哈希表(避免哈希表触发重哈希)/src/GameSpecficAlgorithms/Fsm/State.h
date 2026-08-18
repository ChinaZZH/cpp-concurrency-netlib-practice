#pragma once

#include <cstdint>
#include <string>
#include "../../Common/FixedPoint.h"
#include "../../Common/FixedPonitMaxFunc.h"
#include "../StateContext.h"




// 状态基类
class State
{
public:
    virtual ~State() = default;

    // 状态生命周期函数
    virtual void OnEnter(StateContext& ctx) = 0;
    virtual State* OnUpdate(StateContext& ctx, float delta_ms) = 0;
    virtual void OnExit(StateContext& ctx) = 0;

    // 获取状态名称（用于调试日志）
    virtual std::string GetName() const = 0;

    // 是否可以切换到其他状态（默认允许）
    virtual bool CanTransition() const { return true; }
};