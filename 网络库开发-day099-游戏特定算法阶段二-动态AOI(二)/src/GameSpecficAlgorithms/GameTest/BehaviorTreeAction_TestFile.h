#pragma once

#include <string>
#include "../../Common/FixedPoint.h"
#include "../../Common/FixedPonitMaxFunc.h"
#include "../BehaviorTree/BehaviorTree.h"

class BehaviorTreeAction_TestFile
{
public:
    explicit BehaviorTreeAction_TestFile() = default;
    ~BehaviorTreeAction_TestFile() = default;

public:
    void TestAllScenes();

private:
    void PrintStatus(const std::string& msg, BTStatus status);

    bool ApproxEqual(Fixed a, Fixed b, float epsilon = 0.001f);

private:
    void TestCheckTargetExistsCondition();
    void TestCheckDistanceCondition();
    void TestMoveToTargetAction();
    
    void TestAttackAction();
    void TestPatrolAction();
    void TestFullAITree();
};