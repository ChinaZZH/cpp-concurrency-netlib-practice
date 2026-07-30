#pragma once

#include "../BehaviorTree/BehaviorTree.h"
#include "../BehaviorTree/BTNode.h"
#include <vector>
#include <memory>

// ===== 测试用叶子节点 =====
// 固定返回 Success
class SuccessNode : public BTNode {
public:
    virtual BTStatus Execute(StateContext&, float) override { return BTStatus::Success; }
    virtual std::string GetName() const override { return "Success"; }
};

// 固定返回 Failure
class FailureNode : public BTNode {
public:
    virtual BTStatus Execute(StateContext&, float) override { return BTStatus::Failure; }
    virtual std::string GetName() const override { return "Failure"; }
};

// 固定返回 Running（模拟多帧执行）
class RunningNode : public BTNode {
public:
    virtual BTStatus Execute(StateContext&, float) override { return BTStatus::Running; }
    virtual std::string GetName() const override { return "Running"; }
};


// 返回 Running，并在执行一定次数后返回 Success
class RunningThenSuccessNode : public BTNode {
public:
    RunningThenSuccessNode(int max_steps = 2) : max_steps_(max_steps) {}
    virtual BTStatus Execute(StateContext&, float) override {
        if (step_ < max_steps_) {
            step_++;
            return BTStatus::Running;
        }
        return BTStatus::Success;
    }
    virtual void ResetNode() override { step_ = 0; }
    virtual std::string GetName() const override { return "RunningThenSuccess"; }
private:
    int step_ = 0;
    int max_steps_ = 2;
};