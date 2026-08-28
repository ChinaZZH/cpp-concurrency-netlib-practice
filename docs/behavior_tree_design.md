markdown

# 行为树（Behavior Tree）设计与实现

## 概述
行为树（Behavior Tree）是一种用于控制 AI 行为的层级化决策框架。与有限状态机（FSM）不同，行为树通过组合节点、装饰节点和叶子节点的树形结构来表达 AI 的决策逻辑，具有更好的可扩展性和可维护性。

本文档描述了从零实现的一个轻量级行为树框架，包含组合节点（Sequence/Selector/Parallel）、装饰节点（Inverter/Repeater/UntilSuccess/Timeout/Cooldown）以及具体的行为节点（MoveToTarget/Attack/Patrol），并提供了完整的测试用例验证。

### 适用场景
- 游戏 NPC / 怪物 AI（巡逻、追击、攻击、协作）

- 需要复杂条件判断和并行行为的 AI 系统

- 对可扩展性要求较高的 AI 逻辑

## 为什么用行为树而不是 FSM？

| 维度 | FSM | 行为树 |
| :--- | :--- | :--- |
| 复杂度增长 | 状态数量增加时，转移条件组合爆炸 | 新增节点不影响已有节点，易于扩展 |
| 并行行为 | 难以表达（需引入额外状态） | 原生支持 `Parallel` 组合节点 |
| 行为复用 | 状态逻辑难以复用 | 子节点可在不同树中复用 |
| 调试追踪 | 状态切换路径难以追踪 | 树形结构清晰，返回状态易于观察 |
| 适用场景 | 简单 AI（< 10 个状态） | 复杂 AI（巡逻+追击+攻击+协作+撤退等） |


## 核心概念

### 节点执行状态

每个节点执行后返回以下三种状态之一：

| 状态 | 含义 | 后续行为 |
| :--- | :--- | :--- |
| **Success** | 节点执行成功 | 父节点根据组合逻辑决定下一步 |
| **Failure** | 节点执行失败 | 父节点根据组合逻辑决定下一步 |
| **Running** | 节点正在执行中（需要多帧完成） | 父节点保持该节点，下一帧继续执行 |

`Running` 状态是行为树与 FSM 的核心区别之一。FSM 中状态切换是瞬时的，而行为树允许一个动作跨越多个帧（如移动一段距离），期间持续返回 `Running`，直到完成或失败。

### 节点类型

行为树由以下三类节点组成：

| 节点类型 | 功能 | 子节点数量 | 示例 |
| :--- | :--- | :--- | :--- |
| **组合节点（Composite）** | 控制子节点的执行顺序和逻辑组合 | 多个 | Sequence、Selector、Parallel |
| **装饰节点（Decorator）** | 修改单个子节点的行为（反转、重复、超时等） | 1 个 | Inverter、Repeater、Timeout |
| **叶子节点（Leaf）** | 执行具体行为或判断条件 | 0 个 | MoveToTarget、Attack、CheckDistance |

## 模块结构
```text
BehaviorTree/
├── BTNode.h # 节点基类与状态枚举
├── BTCompositeNode.h # 组合节点基类
├── BTDecoratorNode.h # 装饰节点基类
├── BTConditionNode.h # 条件节点基类
├── BTActionNode.h # 动作节点基类
├── CompositeNodes.h # Sequence / Selector / Parallel
├── DecoratorNodes.h # Inverter / Repeater / UntilSuccess / Timeout / Cooldown
├── Conditions.h # CheckTargetExists / CheckDistance
└── Actions.h # MoveToTarget / Attack / Patrol

```

