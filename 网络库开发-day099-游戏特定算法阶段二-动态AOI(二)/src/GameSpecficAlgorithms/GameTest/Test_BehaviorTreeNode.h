#pragma once

#include "../BehaviorTree/BehaviorTree.h"
#include "../BehaviorTree/BTNode.h"
#include <vector>
#include <memory>
#include <iostream>
#include <cassert>

////////////////////////////////////////////////////////////////////////////////////////////////
////////   用于测试组合结点的
////////////////////////////////////////////////////////////////////////////////////////////////

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


////////////////////////////////////////////////////////////////////////////////////////////////
////////   用于测试装饰结点的
////////////////////////////////////////////////////////////////////////////////////////////////

// 辅助：返回固定状态的测试节点
class TestNode : public BTNode {
public:
    explicit TestNode(BTStatus status, const std::string& name = "Test")
        : status_(status), name_(name) {}
    virtual BTStatus Execute(StateContext&, float) override { return status_; }
    virtual std::string GetName() const override { return name_; }
private:
    BTStatus status_;
    std::string name_;
};

// 计数节点：每次执行增加计数器，第 N 次返回 Success
class CounterNode : public BTNode {
public:
    CounterNode(int target_count) : target_count_(target_count) {}
    virtual BTStatus Execute(StateContext&, float) override {
        count_++;
        if (count_ >= target_count_) return BTStatus::Success;
        return BTStatus::Failure;
    }
    virtual void ResetNode() override { count_ = 0; }
    virtual std::string GetName() const override { return "Counter"; }
private:
    int target_count_;
    int count_ = 0;
};


// 运行中节点（多帧执行）
class RunningThenDoneNode : public BTNode {
public:
    RunningThenDoneNode(int steps = 3) : steps_(steps) {}
    virtual BTStatus Execute(StateContext&, float) override {
        if (step_ < steps_) {
            step_++;
            return BTStatus::Running;
        }
        return BTStatus::Success;
    }
    virtual void ResetNode() override { step_ = 0; }
    virtual std::string GetName() const override { return "RunningThenDone"; }
private:
    int steps_;
    int step_ = 0;
};

