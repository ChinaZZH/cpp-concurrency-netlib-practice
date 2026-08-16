#include "StateMachine.h"
#include "State.h"
#include <iostream>
#include <sstream>

 StateMachine::StateMachine() = default;

    
 StateMachine::~StateMachine() = default;


// 初始化：设置初始状态
void StateMachine::Init(State* initial_state, StateContext& ctx)
{
    current_state_ = std::unique_ptr<State>(initial_state); 
    ctx_ = ctx;
    initialized_ = true;

    if(current_state_)
    {
        current_state_->OnEnter(ctx_);
        std::cout << "[FSM] Entered state: " << current_state_->GetName() << std::endl;
    }
}

// 每帧更新（由游戏主循环调用）
void StateMachine::Update(float delta_ms)
{
    if(false == initialized_ || !current_state_)
    {
        return;
    }

    // 执行当前状态的更新，并获取“下一步”的指令
    State* next_state = current_state_->OnUpdate(ctx_, delta_ms);

    // 如果返回了非空指针，则执行切换
    if(next_state)
    {
        ChangeState(next_state);
    }
}


// 强制切换（外部触发，如受击、死亡）
void StateMachine::ForceChangState(State* newState)
{
    if(newState)
    {
        ChangeState(newState);
    }
}


// 切换状态（由 State::TransitionTo 调用）
void StateMachine::ChangeState(State* new_state)
{
    if(false == initialized_ || !new_state)
    {
        return;
    }

    // 状态不能切换
    if(current_state_ && !current_state_->CanTransition())
    {
        return;
    }

    // 退出当前状态
    if(current_state_)
    {
        std::cout << "[FSM] Exiting state: " << current_state_->GetName() << std::endl;
        current_state_->OnExit(ctx_);
    }

    // 切换到新状态
    current_state_ = std::unique_ptr<State>(new_state); 
    current_state_->OnEnter(ctx_);
    std::cout << "[FSM] Entered state: " << current_state_->GetName() << std::endl;
}


// 获取当前状态名称
std::string StateMachine::GetCurrentStateName() const
{
    if(current_state_)
    {
        return current_state_->GetName();
    }

    return "None";
}

    

// 注册状态（用于状态工厂，可选）
void StateMachine::RegisterState(const std::string& name, State* state)
{
    registered_states_[name] = state;
}