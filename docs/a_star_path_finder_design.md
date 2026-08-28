markdown

# A* 寻路模块设计文档

## 概述

A-Star 寻路模块为游戏 AI 提供了在网格地图上的最优路径规划能力。包括成核心算法实现和在路径平滑与 AI 行为树集成。

### 适用场景

-- 1.  2D 网格地图上的单位寻路

-- 2.  需要绕过静态障碍物的路径规划

-- 3.  与行为树（Behavior Tree）集成的 AI 移动控制

-- 4.   支持四方向/八方向移动

---

## 一、核心设计

### 1.1 算法原理

A-Star算法本质上是 **Dijkstra + 贪婪最佳优先** 的整合：

-- 1.  - 用 `g(n)`（起点到当前点的实际代价）保证路径的最优性

-- 2.  - 用 `h(n)`（当前点到目标的估计代价）引导搜索方向

-- 3.  -  `f(n) = g(n) + h(n)` 作为节点扩展的优先级

-- 4.  当 `h(n) = 0` 时退化为 Dijkstra，当忽略 `g(n)` 时退化为贪婪最佳优先。

### 1.2 启发式函数

支持三种启发式类型，通过枚举切换：

| 类型 | 公式 | 适用场景 |
| :--- | :--- | :--- |
| **曼哈顿距离** | abs(dx) + abs(dy) | 四方向移动（上下左右） |
| **欧几里得距离** | `sqrt(dx² + dy²)` | 自由方向移动 |
| **对角线距离** | `min(dx,dy) * √2 + abs(dx-dy) | 八方向移动 |

---

## 二、数据结构

### 2.1 GridMap（网格地图）

```cpp
class GridMap {
    uint32_t width_, height_;
    float cell_size_;
    std::vector<GridCell> cells_;  // 一维连续存储
};
```

-- 1.  一维 std::vector 存储，保证内存连续性，CPU 缓存友好

-- 2.  支持世界坐标 ↔ 网格坐标转换

-- 3.  支持障碍物标记与查询

### 2.2 GridNode（网格节点）

```cpp
struct GridNode {
    int32_t x, y;
    float g, h, f;
    GridNode* parent;
    bool in_open_list, in_closed_list;
};
```

-- 1.  每个节点存储其在搜索过程中的状态

-- 2.  parent 指针用于路径回溯

-- 3.  in_open_list / in_closed_list 避免重复处理


### 2.3 NodeManager（节点管理器）

```cpp
class NodeManager {
    std::vector<GridNode> nodes_;
    GridNode* GetNode(uint32_t x, uint32_t y);
    void ResetAll();
};
```

-- 1.  统一管理所有节点状态

-- 2.  一次 ResetAll() 清空整个搜索上下文

-- 3.  O(1) 节点访问


## 三、搜索流程

### 3.1 主循环

```text
1. 初始化
   - 起点加入开放列表（g=0，h=启发式值，f=g+h）
   - 关闭列表为空

2. 循环（直到开放列表为空 或 找到终点）
   a. 从开放列表中取出 f 值最小的节点（当前节点）
   b. 将当前节点移入关闭列表
   c. 如果当前节点 == 终点，跳出循环
   d. 遍历当前节点的所有邻居：
      - 如果邻居不可行走 或 在关闭列表中，跳过
      - 计算通过当前节点到达邻居的 g 值
      - 如果邻居不在开放列表中，加入
      - 如果邻居已在开放列表中且新的 g 值更小，更新 g 值和父指针

3. 重构路径
   - 从终点开始，通过 parent 指针回溯到起点
   - 反转路径得到从起点到终点的顺序
 ```
 
 ### 3.2 开放列表实现
 
 使用 std::priority_queue 最小堆，按 f 值从小到大排序。
 
 
 ## 四、路径平滑
 
 原始 A-Star 输出的是网格路径，直接使用会导致 AI 移动呈现锯齿状。模块通过 String Pulling（直线可视性检查） 进行优化：
 
 ```cpp
 class PathSmoother {
    std::vector<PathPoint> SmoothPath(const std::vector<PathPoint>& path);
    bool HasLineOfSight(int32_t x1, int32_t y1, int32_t x2, int32_t y2) const;
};

```
平滑前后对比：

|原始路径 | 平滑后路径 |
|-------	|----------|
|7 个拐点	| 3 个拐点|
|移动时频繁转向 |	移动时保持直线|
|路径点密集 |	路径点稀疏|


 ## 五、与行为树集成
 
 MoveToTargetAction 将 A-Star寻路嵌入到行为树节点中：
 
  ```cpp
 BTStatus MoveToTargetAction::Execute(StateContext& ctx, float delta_ms) {
    // 1. 检查目标是否存在
    // 2. 检查是否需要重新寻路
    // 3. 执行 A* 寻路
    // 4. 沿路径移动
    // 5. 到达时返回 Success
}

```

 ### 5.1 重新寻路触发条件

-- 1.  首次执行（路径为空）

-- 2.  目标移动超过 1 单位

-- 3.  目标 ID 发生变化