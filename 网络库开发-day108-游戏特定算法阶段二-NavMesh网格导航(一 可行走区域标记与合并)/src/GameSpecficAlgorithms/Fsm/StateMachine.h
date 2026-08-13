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
    void Update(float delta_ms);

    // 获取当前状态名称
    std::string GetCurrentStateName() const;

    // 获取上下文（供状态读取/修改）
    StateContext& GetStateContext()  { return ctx_; }

    // 注册状态（用于状态工厂，可选）
    void RegisterState(const std::string& name, State* state);

    // 强制切换（外部触发，如受击、死亡）
    void ForceChangState(State* newState);

private:
    void ChangeState(State* new_state);

private:
    std::unique_ptr<State> current_state_;
    StateContext ctx_;
    bool initialized_ = false;

    // 可选：状态注册表（方便调试）
    std::unordered_map<std::string, State*> registered_states_;
};