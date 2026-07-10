#pragma once

#include <vector>
#include <functional>
#include <string>
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
};

// AOI 统一接口
class IAOIManager {
public:
    using SendMsgCallBack = std::function<void(int entityId, const std::string& msg, GameServerMsgType )>;

    virtual ~IAOIManager() = default;

    // 核心接口
    virtual bool AddEntity(int entityId, int x, int y) = 0;
    virtual bool RemoveEntity(int entityId) = 0;
    virtual bool MoveEntity(int entityId, int newX, int newY) = 0;
    
    // 广播回调注入
    virtual void SetSendMessageCallBack(SendMsgCallBack cb) = 0;
    virtual GridCoordResult GetGridPosition(int entityId) const = 0;
    virtual EntityPositionResult GetEntityPosition(int entityId) const = 0;

    // 辅助查询
    virtual std::vector<int> GetNeighbors(int entityId, int radius = 1) const = 0;
    
    virtual std::pair<int, int> GetGridCoord(int x, int y) const = 0;
};