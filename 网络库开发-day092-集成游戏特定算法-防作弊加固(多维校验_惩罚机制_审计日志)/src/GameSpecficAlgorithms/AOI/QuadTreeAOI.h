#pragma once

#include <cmath>
#include <vector>
#include <memory>
#include <unordered_map>
#include <set>
#include "BaseAOIManager.h"


// 四叉树的结点
class QuadTreeNode
{
public: 
    // 矩形区域
    struct Rect {
        int left_bottom_x;  // 左下角的x
        int left_bottom_y;  // 左下角的y
        int width;          // 宽度    
        int height;         // 高度


        Rect(int x, int y, int w, int h) 
        :left_bottom_x(x),left_bottom_y(y)
        ,width(w), height(h)
        { }
        
        bool Contains(int x, int y) const {
            int right_top_x = left_bottom_x + width;
            int right_top_y = left_bottom_y + height;
            return (x >= left_bottom_x && x < right_top_x) && (y >= left_bottom_y && y < right_top_y);
        }

        bool Intersets(const Rect& other) const {
            // 计算不想交的情况
            bool bNotInterset = (left_bottom_x + width < other.left_bottom_x) 
            || (other.left_bottom_x+other.width < left_bottom_x) 
            || (left_bottom_y + height < other.left_bottom_y) 
            || (other.left_bottom_y + other.height < left_bottom_y);

            return (!bNotInterset);
        }
    };


    QuadTreeNode(IAOIManager* aoi, const Rect& boundary, int capacity  = 0, int maxDepth = 8, int depth = 0);
    ~QuadTreeNode() = default;

    bool Insert(int entityId, int x, int y);
    bool Remove(int entityId);
    bool Update(int entityId, int newX, int newY);
    void Query(const Rect& range, std::set<int>& result) const;
    
    // 获取节点内所有实体（用于合并检查）
    size_t GetTotalEntityCount() const { return entityCount_; }
    //void CollectAllEntityId(std::vector<int>& result) const;

private:
    void DivideToSubChild();
    bool Merge(std::vector<int>& result);
    bool CheckOnSameNodeLeaf(int entityId, int newX, int newY);

private:
    IAOIManager* aoi_; // 存原始指针，控制权不在这个结点这边，这边只做数据访问
    Rect    boundary_;
    int     capacity_;
    int     maxDepth_;
    int     depth_;
    bool    isLeaf_;
    int     entityCount_;                               // 缓存当前的结点个数(当不为叶子结点的时候，包含所有子节点的结点个数)
    
    std::vector<int> entityIdList_;                 // 实体ID列表（仅当前节点，不含子节点）
    std::unique_ptr<QuadTreeNode> children_[4];     // 四个子节点：NW(左上), NE(右上), SW(左下), SE(右下)
};

// 四叉树的方式实现aoi
class QuadTreeAOI : public BaseAOIManager
{
public:
    QuadTreeAOI(int worldWidth, int worldHeight, int gridSize, int nodeCapacity, int maxDepth);
    virtual ~QuadTreeAOI();

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
    std::unique_ptr<QuadTreeNode> root_;

     // 实体ID → 实体信息（物理坐标）
    std::unordered_map<int, EntityInfo> entityMap_;
};