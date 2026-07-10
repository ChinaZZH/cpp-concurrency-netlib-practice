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
#include "BaseAOIManager.h"
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


class GridAOI : public BaseAOIManager
{
public:
    explicit GridAOI(int gridSize = 100)
    :BaseAOIManager(gridSize)
    {

    }

    virtual ~GridAOI() {}

    // 接口
    virtual bool AddEntity(int entityId, int x, int y) override;
    virtual bool RemoveEntity(int entityId) override;
    virtual bool MoveEntity(int entityId, int newX, int newY) override;
    
    // 获取周围实体列表（默认九宫格，radius 可扩展）
    virtual std::vector<int> GetNeighbors(int entityId, int radius = 1) const override; 

    // 辅助，根据实体坐标转换为网格坐标
    virtual  GridCoordResult GetGridPosition(int entityId) const override;

    virtual EntityPositionResult GetEntityPosition(int entityId) const override;

private:
    //int gridSize_;

    // 实体ID → 实体信息（物理坐标）
    std::unordered_map<int, EntityInfo> entityMap_;
    
    // 网格坐标 (gridX, gridY) → 该网格内的实体ID集合
    // 优化成使用unordered_set
    std::unordered_map<std::pair<int, int>, std::set<int>, PairHash> gridMap_;

    //SendMsgCallBack sendMsgCallBack_;
};