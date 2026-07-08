#pragma once

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <map>
#include <utility>
#include <algorithm>
#include <string>

// ============================================================
// 实体信息
// ============================================================
struct EntityInfo
{
    int x;
    int y;
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
class AOIManager
{
public:
    explicit AOIManager(int gridSize = 100)
    :gridSize_(gridSize)
    {

    }

    // 接口
    void AddEntity(int entityId, int x, int y);
    void RemoveEntity(int entityId);
    void MoveEntity(int entityId, int newX, int newY);
    
     // 获取周围实体列表（默认九宫格，radius 可扩展）
    std::vector<int> GetNeighbors(int entityId, int radius = 1) const; 

    // 辅助，根据实体坐标转换为网格坐标
    std::pair<int, int> GetGridPostion(int entityId) const;

private:
    std::pair<int, int> GetGridCoord(int x, int y) const;

    bool CheckPositionIsValid(int x, int y, const std::string& strLogContent);

private:
    int gridSize_;

    // 实体ID → 实体信息（物理坐标）
    std::unordered_map<int, EntityInfo> entityMap_;
    
    // 网格坐标 (gridX, gridY) → 该网格内的实体ID集合
    // 优化成使用unordered_set
    std::unordered_map<std::pair<int, int>, std::unordered_set<int>, PairHash> gridMap_;
};