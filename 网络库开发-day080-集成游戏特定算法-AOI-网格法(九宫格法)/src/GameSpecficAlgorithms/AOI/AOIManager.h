#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <map>
#include <utility>
#include <algorithm>
#include <string>
#include <functional>
#include "../GameServerMsgTypeDefine.h"
// ============================================================
// 实体信息
// ============================================================
struct EntityInfo
{
    int x;
    int y;

    int gridX;
    int gridY;
};

// ============================================================
// 哈希函数：支持 std::pair<int, int> 作为 unordered_map 的键
// ============================================================
struct PairHash {
    size_t operator()(const std::pair<int, int>& p) const {
        return std::hash<int>()(p.first) ^ (std::hash<int>()(p.second) << 1);
    }
};

// ============================================================
// AOI 管理器（网格法 / 九宫格）
// ============================================================
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

class AOIManager
{
public:
    using SendMsgCallBack = std::function<void(int , const std::string& , GameServerMsgType )>;

    explicit AOIManager(int gridSize = 100)
    :gridSize_(gridSize)
    {

    }

    // 接口
    bool AddEntity(int entityId, int x, int y);
    bool RemoveEntity(int entityId);
    bool MoveEntity(int entityId, int newX, int newY);
    
     // 获取周围实体列表（默认九宫格，radius 可扩展）
    std::vector<int> GetNeighbors(int entityId, int radius = 1) const; 

    // 辅助，根据实体坐标转换为网格坐标
    GridCoordResult GetGridPostion(int entityId) const;

    EntityPositionResult GetEntityPostion(int entityId) const;

    void SetSendMessageCallBack(SendMsgCallBack cb) { sendMsgCallBack_ = cb; }

private:
    std::pair<int, int> GetGridCoord(int x, int y) const;

    bool CheckPositionIsValid(int x, int y, const std::string& strLogContent);

    void BroadNeighborsEntityList(int entityId, const std::string& strMsg, GameServerMsgType msgType);

    void BroadEntityList(const std::vector<int>& entityList, const std::string& strMsg, GameServerMsgType msgType);

    void EnterNewGridPosMsgNotify(int entityId, const EntityInfo& newEntity, const std::vector<int>& newNeighborsEntitys);

private:
    int gridSize_;

    // 实体ID → 实体信息（物理坐标）
    std::unordered_map<int, EntityInfo> entityMap_;
    
    // 网格坐标 (gridX, gridY) → 该网格内的实体ID集合
    // 优化成使用unordered_set
    std::unordered_map<std::pair<int, int>, std::set<int>, PairHash> gridMap_;

    SendMsgCallBack sendMsgCallBack_;
};