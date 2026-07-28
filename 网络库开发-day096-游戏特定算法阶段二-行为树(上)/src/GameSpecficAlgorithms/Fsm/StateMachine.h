#pragma once

#include <cstdint>
#include <string>
#include <memory>
#include <unordered_map>
#include "State.h"

// 状态机管理器
class StateMachine
{
public:
    StateMachine();
    ~StateMachine();

    // 初始化：设置初始状态
    void Init(State* initial_state, StateContext& ctx);

    // 每帧更新（由游戏主循环调用）
    void Update(uint32_t delta_ms);

    // 切换状态（由 State::TransitionTo 调用）
    void ChangeState(State* new_state);

    // 获取当前状态名称
    std::string GetCurrentStateName() const;

    // 获取上下文（供状态读取/修改）
    StateContext& GetStateContext()  { return ctx_; }


    // 注册状态（用于状态工厂，可选）
    void RegisterState(const std::string& name, State* state);

private:
    std::unique_ptr<State> current_state_;
    StateContext ctx_;
    bool initialized_ = false;

    // 可选：状态注册表（方便调试）
    std::unordered_map<std::string, State*> registered_states_;
};