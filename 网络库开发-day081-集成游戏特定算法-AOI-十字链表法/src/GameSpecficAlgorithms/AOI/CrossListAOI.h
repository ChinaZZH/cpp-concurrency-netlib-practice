#include <cmath>
#include <vector>
#include <memory>
#include <unordered_map>

struct CrossListNode {
    int entityId;
    int x;
    int y;

    std::shared_ptr<CrossListNode> prevX;
    std::shared_ptr<CrossListNode> nextX;
    std::shared_ptr<CrossListNode> prevY;
    std::shared_ptr<CrossListNode> nextY;

    CrossListNode(int id, int px, int py)
    :entityId(id), x(px), y(py)
    ,prevX(nullptr), nextX(nullptr)
    ,prevY(nullptr), nextY(nullptr)
    {

    }
};


class CrossListAOI 
{
public:
    CrossListAOI(int gridSize = 100);

    ~CrossListAOI();

    // 禁止拷贝
    CrossListAOI(const CrossListAOI&) = delete;
    CrossListAOI& operator=(const CrossListAOI&) = delete;

    // 接口
    bool AddEntity(int entityId, int x, int y);
    bool RemoveEntity(int entityId);
    bool MoveEntity(int entityId, int newX, int newY);

     // 获取周围实体列表（默认九宫格，radius 可扩展）
    std::vector<int> GetNeighbors(int entityId, int radius = 1) const; 

private:
    void RemoveNode(std::shared_ptr<CrossListNode> node);
    bool IsInRange(int x1, int y1, int x2, int y2, int radius) const;

    bool InsertNodeForX(std::shared_ptr<CrossListNode> node);
    bool InsertNodeForY(std::shared_ptr<CrossListNode> node);

    bool RemoveNodeForX(int entityId);
    bool RemoveNodeForY(int entityId);

    std::pair<int, int> GetGridCoord(int x, int y) const;

private:
    int gridSize_;
    std::shared_ptr<CrossListNode> headX_ = nullptr;
    std::shared_ptr<CrossListNode> headY_ = nullptr;
    std::unordered_map<int, std::shared_ptr<CrossListNode>> nodeMap_;
};