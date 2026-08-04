#pragma once

#include "BTActionNode.h"


// 攻击目标，一次执行一次攻击（含冷却逻辑可后续扩展）。
class AttackAction : public BTActionNode
{
public:
    explicit AttackAction(float interval_ms = 1000.0f)
    : BTActionNode()
    , interval_ms_(interval_ms)
    , cooldown_remaining_(0.00f)
    { }


    virtual ~AttackAction() = default;

    // 核心执行接口
    // - ctx: 状态上下文（与 FSM 共用 StateContext）
    // - delta_ms: 时间步长（毫秒）
    virtual BTStatus Execute(StateContext& ctx, float delta_ms) override 
    {
        if(0 == ctx.target_id)
        {
            ResetNode();
            return BTStatus::Failure;
        }

       cooldown_remaining_ -= delta_ms;
       if(cooldown_remaining_ <= 0.00f)
       {
            // 实际项目：通过回调或事件系统通知外部
            std::cout << "[BT] AI " << ctx.entity_id << " attacks target " << ctx.target_id << std::endl;
            cooldown_remaining_ = interval_ms_;
            return BTStatus::Success;
       }

       // 冷却未到，返回 Running（等待下一帧继续计数）
       return BTStatus::Running;
    }

    virtual void ResetNode() override 
    {
        cooldown_remaining_ = 0.00f;
    }

    virtual std::string GetName() const override 
    {
        return "Attack";
    }


private:
    float interval_ms_;
    float cooldown_remaining_ = 0.0f;
};