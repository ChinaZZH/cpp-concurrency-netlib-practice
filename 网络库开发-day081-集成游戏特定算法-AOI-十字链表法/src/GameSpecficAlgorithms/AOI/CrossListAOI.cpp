#include "CrossListAOI.h"
#include <iostream>
#include <sstream>
#include <cstdlib> 
#include "../../Common/LogFile.h"
#include "../../../build/proto_gen/aoi.pb.h"


CrossListAOI::CrossListAOI(int gridSize /*= 100*/)
:gridSize_(gridSize)
{ 

}



CrossListAOI::~CrossListAOI()
{

}

// 接口
bool CrossListAOI::AddEntity(int entityId, int x, int y)
{
    // 已经存在该实体，则返回false
    auto itr = nodeMap_.find(entityId);
    if(itr != nodeMap_.end())
    {
        return false;
    }

    std::shared_ptr<CrossListNode> newNode = std::make_shared<CrossListNode>(entityId, x, y);
    if(!newNode)
    {
        return false;
    }

    InsertNodeForX(newNode);
    InsertNodeForY(newNode);
    nodeMap_[entityId] = newNode;

    // 数据同步部分由后面的将和九宫格法相同的代码抽象成基类，那时候又基类实现。
    return true;
}


bool CrossListAOI::RemoveEntity(int entityId)
{
    // 不存在该实体，则返回false
    auto itr = nodeMap_.find(entityId);
    if(itr == nodeMap_.end())
    {
        return false;
    }

   RemoveNodeForX(entityId);
   RemoveNodeForY(entityId);
   nodeMap_.erase(entityId);
   return true;
}

bool CrossListAOI::MoveEntity(int entityId, int newX, int newY)
{
    // 不存在该实体，则返回false
    auto itr = nodeMap_.find(entityId);
    if(itr == nodeMap_.end())
    {
        return false;
    }

    // 先移除
    RemoveNodeForX(entityId);
    RemoveNodeForY(entityId);

    std::shared_ptr<CrossListNode> moveEntity = (itr->second);
    moveEntity->x = newX;
    moveEntity->y = newY;
    moveEntity->prevX = nullptr;
    moveEntity->nextX = nullptr;
    moveEntity->prevY = nullptr;
    moveEntity->nextY = nullptr;

    InsertNodeForX(moveEntity);
    InsertNodeForY(moveEntity);
    return true;
}


// 获取周围实体列表（默认九宫格，radius 可扩展）
std::vector<int> CrossListAOI::GetNeighbors(int entityId, int radius /*= 1*/) const
{
     // 不存在该实体，则返回false
    auto itr = nodeMap_.find(entityId);
    if(itr == nodeMap_.end())
    {
        return std::vector<int>();
    }

    std::set<int> neighborsList;
    const std::shared_ptr<CrossListNode>& node = (itr->second);
    // 找到了，择先向x轴的preV扩展
    {
         std::shared_ptr<CrossListNode> preNode = node->prevX;
         while(preNode)
         {
            if(!IsInRange(node->x, node->y, preNode->x, preNode->y, radius))
            {
                break;
            }

            neighborsList.insert(preNode->entityId);
            preNode = preNode->prevX;
         }
    }

    // 找到了，择先向x轴的next扩展
    {
         std::shared_ptr<CrossListNode> nextNode = node->nextX;
         while(nextNode)
         {
            if(!IsInRange(node->x, node->y, nextNode->x, nextNode->y, radius))
            {
                break;
            }

            neighborsList.insert(nextNode->entityId);
            nextNode = nextNode->nextX;
         }
    }

    // 找到了，择先向y轴的preV扩展
    {
         std::shared_ptr<CrossListNode> preNode = node->prevY;
         while(preNode)
         {
            if(!IsInRange(node->x, node->y, preNode->x, preNode->y, radius))
            {
                break;
            }

            neighborsList.insert(preNode->entityId);
            preNode = preNode->prevY;
         }
    }

    // 找到了，择先向y轴的next扩展
    {
         std::shared_ptr<CrossListNode> nextNode = node->nextY;
         while(nextNode)
         {
            if(!IsInRange(node->x, node->y, nextNode->x, nextNode->y, radius))
            {
                break;
            }

            neighborsList.insert(nextNode->entityId);
            nextNode = nextNode->nextY;
         }
    }

    neighborsList.erase(entityId);

    std::vector<int> vecNeighborsId;
    std::copy(neighborsList.begin(), neighborsList.end(), std::back_inserter(vecNeighborsId));
    return vecNeighborsId;
}



void CrossListAOI::RemoveNode(std::shared_ptr<CrossListNode> node)
{

}
    
bool CrossListAOI::IsInRange(int x1, int y1, int x2, int y2, int radius) const
{
    auto firstGrid = this->GetGridCoord(x1, y1);
    auto secondtGrid = this->GetGridCoord(x2, y2);
    return std::abs(firstGrid.first - secondtGrid.first) <= radius && std::abs(firstGrid.second - secondtGrid.second) <= radius;
}

std::pair<int, int> CrossListAOI::GetGridCoord(int x, int y) const
{
    return std::pair(x / gridSize_, y / gridSize_);
}

bool CrossListAOI::InsertNodeForX(std::shared_ptr<CrossListNode> node)
{
    // 处理x轴的情况
    if(nullptr == headX_)
    {
        headX_ = node;
        return true;
    }

    // 如果headX的本身就大于node的X值
    if((headY_->x) > (node->x))
    {
        node->nextX = headX_;
        headX_->prevX = node;
        headX_ = node;
        return true;
    }


    // 找到第一个x的坐标的素质比node更大的。
    std::shared_ptr<CrossListNode> nodeNextX = headX_;
    while(nodeNextX)
    {
        // 当前nodeNext的X数值比node->x的更大，则node需要挂在nodeNextX的前面
        if((nodeNextX->x) > (node->x))
        {
            break;
        }

        // 下面逻辑是nodeNext的X数值node->x的更小的逻辑，如果已经没有办法往后走了，说明当前已经是最大值了，则break，将node挂在最后面
        if(!nodeNextX->nextX)
        {
            nodeNextX->nextX = node;
            node->prevX = nodeNextX;
            return true;
        }

        nodeNextX = nodeNextX->nextX;
    }

    // 已经找到了第一个x数值高于node的结点
    std::shared_ptr<CrossListNode> preNode = nodeNextX->prevX;
    if(!preNode)
    {
        // 前面已经将node是head结点考虑进去了，则这边就是错误判断了
        return false;
    }

    // 先将node和nodeNext进行连接，
    node->nextX = nodeNextX;
    nodeNextX->prevX = node;

    // 处理node前置结点的情况
    preNode->nextX = node;
    node->prevX = preNode;
    return true;
}
    

bool CrossListAOI::InsertNodeForY(std::shared_ptr<CrossListNode> node)
{
    // 处理y轴的情况
    if(nullptr == headY_)
    {
        headY_ = node;
        return true;
    }

    // 如果headY的本身就大于node的Y值
    if((headY_->y) > (node->y))
    {
        node->nextY = headY_;
        headY_->prevY = node;
        headY_ = node;
        return true;
    }


    // 找到第一个x的坐标的素质比node更大的。
    std::shared_ptr<CrossListNode> nodeNextY = headY_;
    while(nodeNextY)
    {
        // 当前nodeNext的y数值比node->y的更大，则node需要挂在nodeNext的前面
        if((nodeNextY->y) > (node->y))
        {
            break;
        }

        // 下面逻辑是nodeNext的y数值node->y的更小的逻辑，如果已经没有办法往后走了，说明当前已经是最大值了，将新结点挂在node挂在最后面.
        if(!nodeNextY->nextY)
        {
            nodeNextY->nextY = node;
            node->prevY = nodeNextY;
            return true;
        }

        nodeNextY = nodeNextY->nextY;
    }


    // 已经找到了第一个x数值高于node的结点
    std::shared_ptr<CrossListNode> preNOde = nodeNextY->prevY;
    if(!preNOde)
    {
        // 前面已经将node是head结点考虑进去了，则这边就是错误判断了
        return false;
    }

    // 先将node和nodeNext进行连接
    node->nextY = nodeNextY;
    nodeNextY->prevY = node;

    // 处理node前置结点和node的连接
    preNOde->nextY = node;
    node->prevY = preNOde;
    return true;
}


bool CrossListAOI::RemoveNodeForX(int entityId)
{
    if(!headX_)
    {
        return false;
    }

    // 处理头结点就是entityId的情况
    if(headX_->entityId == entityId)
    {
        std::shared_ptr<CrossListNode> ptrOldHead = headX_;
        headX_ = headX_->nextX;
        
        ptrOldHead->nextX = nullptr;
        return true;
    }

    std::shared_ptr<CrossListNode> findNode = headX_;
    while(findNode)
    {
        if(findNode->entityId == entityId)
        {
            break;
        }

        findNode = findNode->nextX;
    }

    if(!findNode)
    {
        return false;
    }

    // 头结点的情况已经处理，如果preNode没有值，那就是错误
    std::shared_ptr<CrossListNode> preNode = findNode->prevX;
    if(!preNode)
    {
        return false;
    }

    std::shared_ptr<CrossListNode> nextNode = findNode->nextX;
    preNode->nextX = nextNode;
    if(nextNode)
    {
        nextNode->prevX = preNode;
    } 

    // 将findNode的连接清空
    findNode->prevX = nullptr;
    findNode->nextX = nullptr;
    return true; 
}


bool CrossListAOI::RemoveNodeForY(int entityId)
{
    if(!headY_)
    {
        return false;
    }

    // 处理头结点就是entityId的情况
    if(headY_->entityId == entityId)
    {
        std::shared_ptr<CrossListNode> ptrOldHead = headY_;
        headY_ = headY_->nextY;
        
        ptrOldHead->nextY = nullptr;
        return true;
    }

    std::shared_ptr<CrossListNode> findNode = headY_;
    while(findNode)
    {
        if(findNode->entityId == entityId)
        {
            break;
        }

        findNode = findNode->nextY;
    }

    if(!findNode)
    {
        return false;
    }

    // 头结点的情况已经处理，如果preNode没有值，那就是错误
    std::shared_ptr<CrossListNode> preNode = findNode->prevY;
    if(!preNode)
    {
        return false;
    }

    std::shared_ptr<CrossListNode> nextNode = findNode->nextY;
    preNode->nextY = nextNode;
    if(nextNode)
    {
        nextNode->prevY = preNode;
    } 

    // 将findNode的连接清空
    findNode->prevY = nullptr;
    findNode->nextY = nullptr;
    return true; 
}