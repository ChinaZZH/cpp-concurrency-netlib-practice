markdown


# Day97： 游戏特定算法阶段二   行为树(下)

## 核心收获

-- 1.  行为树 行为节点(为叶子节点)，控制具体行为。

-- 1.1.  AttackAction  攻击目标行为结点。一次执行一次攻击（含冷却逻辑）。

-- 1.2.  MoveToTargetAction  向目标移动行为结点，到达后返回 Success，目标丢失返回 Failure。  

-- 1.3.  PatrolAction  巡逻行为结点。 在随机巡逻点之间循环移动。


-- 2.  行为树 条件结点，判断条件是否满足。

-- 2.1.  CheckDistanceCondition  检查到目标距离是否小于阈值。

-- 2.2.  CheckTargetExistsCondition  检查目标是否存在


-- 3.  装饰结点  管理单个子节点。 装饰节点是行为树中的“修饰器”，它们不改变子节点的核心行为，而是在子节点执行前后或执行过程中附加额外的控制逻辑，例如：限制执行次数、反转结果、添加冷却时间、设置超时等。

-- 3.1.  UntilSuccessNode 不断执行子节点直到返回 Success。

-- 3.2.  TimeoutNode 限制子节点执行时间，超时则返回 Failure。
 
-- 3.3.  RepeaterNode 重复执行子节点 N 次。

-- 3.4.  InverterNode 反转子节点的 Success/Failure。

-- 3.4.  CooldownNode 限制节点执行频率（冷却时间）

### 测试
-- 1.  测试通过。


## 测试
-- 一切正常。