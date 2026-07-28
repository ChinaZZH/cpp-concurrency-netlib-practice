markdown


# 有限状态机（FSM）设计与实现

## 概述

-- 1. 本文档描述了一个轻量级的确定性有限状态机（FSM）框架，专为游戏 AI 行为管理而设计。

-- 2. 该框架支持状态的生命周期管理（进入 / 更新 / 退出）、上下文数据传递，以及基于返回值的状态切换机制。


### 适用场景

-  游戏 NPC / 怪物 AI（巡逻、追击、攻击、待机）

-  角色状态管理（待机、跑动、跳跃、攻击）

-  任何需要明确状态流转且状态数量可控的业务逻辑

### 核心设计原则

- **上下文解耦**：状态本身不持有对游戏对象的直接引用，所有外部数据通过 `StateContext` 传递，便于单元测试和逻辑复用。

- **状态机驱动切换**：状态不持有状态机指针，通过 `OnUpdate` 返回下一个状态的方式实现切换，职责清晰，依赖方向单一。

- **确定性运算**：坐标计算、距离判定均基于定点数（`Fixed`），确保跨平台行为一致性。

## 模块结构

```text
fsm/
├── State.h # 状态基类与上下文定义
├── StateMachine.h/cpp # 状态机管理器
└── states/
├── IdleState.h/cpp # 待机状态
├── PatrolState.h/cpp # 巡逻状态
├── ChaseState.h/cpp # 追击状态
└── AttackState.h/cpp # 攻击状态
```

## 核心类设计

### StateContext（状态上下文）

-  上下文是状态与外部世界的“数据通道”，包含 AI 实体的必要信息。

-  通过传递上下文，状态可以读取和修改实体的位置、血量、目标等信息，而无需直接持有实体引用。

```cpp
struct StateContext {
    uint32_t entity_id;         // 实体 ID
    Fixed x, y;                 // 当前位置
    Fixed hp;                   // 血量
    uint32_t target_id;         // 目标实体 ID（0 表示无目标）
    Fixed target_x, target_y;   // 目标位置
};
```

### State（状态基类）

-  每个状态类需要实现三个生命周期函数和一个状态切换检查函数：

```cpp
class State {
public:
    virtual void OnEnter(StateContext& ctx) = 0;
    virtual State* OnUpdate(StateContext& ctx, float delta_ms) = 0;
    virtual void OnExit(StateContext& ctx) = 0;
    virtual bool CanTransition() const { return true; }
};
```

OnUpdate 的返回值是关键——返回新状态的实例，表示切换到该状态；返回 nullptr 表示保持当前状态

### StateMachine（状态机管理器）

-状态机负责：

-  管理当前状态的生命周期（由 std::unique_ptr 自动管理）。

-  执行状态的 OnUpdate，并根据返回值自动切换。

-  对外暴露 Update 驱动入口和上下文访问接口。

###  状态流转图
graph TD
    I[Idle] -->|空闲超时| P[Patrol]
    P -->|发现目标| C[Chase]
    C -->|进入攻击范围| A[Attack]
    A -->|目标跑远| C
    A -->|目标消失| I
    C -->|目标太远或消失| I
	
	
###  状态说明

|状态|	行为描述|	切换条件|
|----|------------|----------|
|Idle|	原地等待，持续 1~3秒|	空闲超时 → Patrol；发现目标 → Chase|
|Patrol|	在随机巡逻点之间移动|	发现目标 → Chase|
|Chase|	向目标移动|	进入攻击范围（< 50 单位）→ Attack；目标太远/消失 → Idle|
|Attack|	周期性攻击目标（1 秒冷却）|	目标跑远（> 70 单位）→ Chase；目标消失 → Idle|

## 关键实现细节

###  返回切换模式

-   状态本身不持有状态机指针，切换逻辑由状态机统一处理。状态通过 OnUpdate 返回下一个状态，状态机负责执行实际的切换操作。

-  这种设计的优点是解耦，缺点是每次切换都会 new 一个新状态，但考虑到 AI 状态数量有限且切换频率不高，可接受。

```cpp
// 状态中触发切换
if (distance < attackRange) {
    return new AttackState();
}

// 状态机中的处理
State* next = current_state_->OnUpdate(ctx, delta_ms);
if (next) {
    ChangeState(next);
}

```

###  定点数运算

-  所有坐标计算和距离判定均使用定点数，保证跨平台确定性：

```cpp
// 计算距离平方
Fixed DistanceToTargetSq() const {
    Fixed dx = x - target_x;
    Fixed dy = y - target_y;
    return dx * dx + dy * dy;
}

// 使用 FixedSqrt 计算实际距离
Fixed dist = FixedMath::FixedSqrt(distSq);

```

###   状态切换滞后（推荐优化）

- 为避免在边界附近频繁切换，建议在 Chase 和 Attack 之间使用不同的进入/退出阈值：

```cpp
// Chase → Attack：进入阈值 45^2（更严格）
if (distSq < 45 * 45) return new AttackState();

// Attack → Chase：退出阈值 80^2（更宽松）
if (distSq > 80 * 80) return new ChaseState();

```