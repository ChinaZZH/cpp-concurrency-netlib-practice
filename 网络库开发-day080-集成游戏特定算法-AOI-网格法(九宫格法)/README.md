markdown

# Day80： 集成游戏特定算法  AOI网格法(九宫格法)

## 核心收获

### 九宫格法和其他AOI算法的对比 

|算法|	核心特点|	最佳适用场景|	慎用场景|
|-----|-----------|---------------|-----------|
|网格法（九宫格）|	简单直接，按位置划分固定格子	| 平坦开阔地图、玩家分布相对均匀（如 MOBA、大逃杀）|	玩家高度聚集（如主城摆摊）、地图地形复杂多障碍|
|十字链表|	基于双向链表维护 X/Y 轴顺序，擅长遍历|	逻辑密集型场景、需要快速 AOI 遍历（如回合制游戏、策略游戏）|	高频创建/删除实体（如频繁进出副本）|
|四叉树|	自适应分区，根据对象密度动态调整	| 地形复杂、对象分布不均匀（如开放世界 RPG、场景中有大量障碍物）|	需要极高性能的实时竞技游戏（因为动态分区和移动更新开销较大）|

###  核心数据结构（AOIManager）
-- 1.  实现了网格法九宫格的核心逻辑：AddEntity、RemoveEntity、MoveEntity、GetNeighbors

-- 2.  用 std::set 存储每个网格内的实体，使得 std::set_intersection / std::set_difference 可以直接使用

-- 3.   缓存了实体物理坐标 + 网格坐标，避免每次移动都重新计算

-- 4.    实现了空网格自动清理，避免数据堆积

-- 5.    设计了 GridCoordResult / EntityPositionResult 返回值结构，替代了可读性差的嵌套 pair

###  Protobuf 协议设计
-- 1.  定义了 4 个消息类型：EntityEnterNotifyResponse、InitAroundEntitiesNotifyResponse、EntityLeaveNotifyResponse、EntityMoveNotifyResponse

-- 2. 区分了两个方向：新实体→周围实体（通知“我来了”），周围实体→新实体（通知“你周围有谁”）

###  GameServer 集成层
-- 1.  GameServer 继承自 RpcServer，复用网络库的消息解码和 RPC 框架

-- 2.  实现了 LengthAndTypePrefixDecoder，支持“长度 + 类型”的拆包协议

-- 3.   注册了 GSMT_AddEntity、GSMT_RemoveEntity、GSMT_MoveEntity 三个消息处理器

-- 4.    实现了 onServerConnections_ 管理实体ID → TCP连接 + EventLoop 的映射


###  线程模型
-- 1.  消息处理通过 分片线程池 异步执行，I/O 线程只负责收发

-- 2.  广播消息通过 EventLoop::RunInLoop 转回 I/O 线程发送

-- 3.   工作线程执行 AOI 逻辑，通过 “地图id取模到特定线程”是一种基于数据亲和性的负载均衡策略，常用于避免同一数据的频繁加锁或跨线程通信。

-- 4.    通过 “绑核 + 分片”的方式 让逻辑在线程池中的线程调度，同时又能保证统一地图的逻辑在同一线程内顺序执行，避免使用锁机制加大系统的复杂度。



### 广播逻辑
-- 1.  AddEntity：向周围实体发送“有新实体进入”，同时向新实体发送“你周围有哪些实体”

-- 2.  MoveEntity：分为三种情况——网格未变（只广播位置变化）、网格变化（区分“仍在视野内”“新进入视野”“离开视野”三类事件）

-- 3. RemoveEntity：向周围实体广播“该实体已离开”

### 测试
-- 1.  单元测试 AlgorithmsUnitTesting::TestAoiManager 验证了添加、移动、删除、查询的基本流程

-- 2.   通过了客户端和服务端消息收发和打印验证了逻辑的正确性。

## 结论

--  1. 新增，删除，移动，都是通过哈希表O(1)性能定位，性能良好。

-- 2.  查找视野内实体也是通过固定九宫格视野内范围去遍历，没有多做额外的不必要的工作。

--  3. 实现简单，易懂易维护。

## 测试
-- 一切正常。


