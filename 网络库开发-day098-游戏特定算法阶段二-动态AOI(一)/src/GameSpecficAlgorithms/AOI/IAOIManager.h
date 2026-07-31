#pragma once

#include <vector>
#include <functional>
#include <string>
#include <chrono>
#include <memory>
#include "../GameServerMsgTypeDefine.h"

// 网格查询结果
struct GridCoordResult {
    bool valid = false;
    int gridX = 0;
    int gridY = 0;
};

struct EntityPositionResult {
    bool valid = false;
    int x = 0;
    int y = 0;
    std::chrono::steady_clock::time_point lastUpdateTime;
};

// AOI 统一接口
class PartitionedPool;
class DeltaSyncManager;

class IAOIManager {
public:
    using SendMsgCallBack = std::function<void(int entityId, const std::string& msg, GameServerMsgType )>;

    virtual ~IAOIManager() = default;
    

    // 核心接口
    virtual bool AddEntity(int entityId, int x, int y) = 0;
    virtual bool RemoveEntity(int entityId) = 0;
    virtual bool MoveEntity(int entityId, int newX, int newY) = 0;
    
    // 广播回调注入
    virtual EntityPositionResult GetEntityPosition(int entityId) const = 0;

    // 辅助查询
    virtual std::vector<int> GetNeighbors(int entityId, int radius = 1) const = 0;
};