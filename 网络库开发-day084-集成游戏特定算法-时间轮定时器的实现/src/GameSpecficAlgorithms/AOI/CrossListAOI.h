#include <cmath>
#include <vector>
#include <memory>
#include <unordered_map>
#include "BaseAOIManager.h"

struct CrossListNode {
    int entityId;
    int x;
    int y;
    int gridX;
    int gridY;

    std::shared_ptr<CrossListNode> prevX;
    std::shared_ptr<CrossListNode> nextX;
    std::shared_ptr<CrossListNode> prevY;
    std::shared_ptr<CrossListNode> nextY;

    CrossListNode(int id, int px, int py, int gx, int gy)
    :entityId(id), x(px), y(py), gridX(gx), gridY(gy)
    ,prevX(nullptr), nextX(nullptr)
    ,prevY(nullptr), nextY(nullptr)
    {
        
    }
};

// 十字链表方式实现aoi
class CrossListAOI : public BaseAOIManager
{
public:
    CrossListAOI(int gridSize = 100)
    :BaseAOIManager(gridSize)
    {

    }

    virtual ~CrossListAOI() {}

    // 禁止拷贝
    CrossListAOI(const CrossListAOI&) = delete;
    CrossListAOI& operator=(const CrossListAOI&) = delete;

    // 接口
    virtual bool AddEntity(int entityId, int x, int y) override;
    virtual bool RemoveEntity(int entityId) override;
    virtual bool MoveEntity(int entityId, int newX, int newY) override;

     // 获取周围实体列表（默认九宫格，radius 可扩展）
    std::vector<int> GetNeighbors(int entityId, int radius = 1) const override; 

    // 辅助，根据实体坐标转换为网格坐标
    virtual  GridCoordResult GetGridPosition(int entityId) const override;

    virtual EntityPositionResult GetEntityPosition(int entityId) const override;

private:
    bool InsertNodeForX(std::shared_ptr<CrossListNode> node);
    bool InsertNodeForY(std::shared_ptr<CrossListNode> node);

    bool RemoveNodeForX(int entityId);
    bool RemoveNodeForY(int entityId);

    bool MoveNodeToNewX(int entityId, int newX, int newGridX);

    bool MoveNodeToNewY(int entityId, int newY, int newGridY);

private:
    std::shared_ptr<CrossListNode> headX_ = nullptr;
    std::shared_ptr<CrossListNode> headY_ = nullptr;
    std::unordered_map<int, std::shared_ptr<CrossListNode>> nodeMap_;
};