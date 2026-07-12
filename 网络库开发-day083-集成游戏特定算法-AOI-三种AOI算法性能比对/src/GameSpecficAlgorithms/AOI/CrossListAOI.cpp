#include "CrossListAOI.h"
#include <iostream>
#include <sstream>
#include <cstdlib> 
#include "../../Common/LogFile.h"
#include "../../../build/proto_gen/aoi.pb.h"


// 接口
bool CrossListAOI::AddEntity(int entityId, int x, int y)
{
    // 已经存在该实体，则返回false
    auto itr = nodeMap_.find(entityId);
    if(itr != nodeMap_.end())
    {
        return false;
    }

    auto gridPositon = BaseAOIManager::GetGridCoord(x, y);
    std::shared_ptr<CrossListNode> newNode = std::make_shared<CrossListNode>(entityId, x, y, gridPositon.first, gridPositon.second);
    if(!newNode)
    {
        return false;
    }

    InsertNodeForX(newNode);
    InsertNodeForY(newNode);
    nodeMap_[entityId] = newNode;

   // 广播有新entity消息下发newEntityResponse，发送entityId, x, y
    if(msgNotifyer_)
    {
        auto neighborsEntityList = this->GetNeighbors(entityId);
        msgNotifyer_->EnterNewGridPosMsgNotify(entityId, x, y, neighborsEntityList);
    }

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

   auto neighborsEntityList = this->GetNeighbors(entityId);
   RemoveNodeForX(entityId);
   RemoveNodeForY(entityId);
   
 
   if(msgNotifyer_)
   {
        msgNotifyer_->LeaveGridToMsgNotify(entityId, neighborsEntityList);
   }

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

    // 位置没有发生变化的时候，直接返回
     std::shared_ptr<CrossListNode> moveEntity = (itr->second);
    if(moveEntity->x == newX && moveEntity->y == newY)
    {
        return true;
    }


    std::pair<int, int> oldGridPos(moveEntity->gridX, moveEntity->gridY);
    auto oldNeighborsEntitys = this->GetNeighbors(entityId);

    // 先移除
    RemoveNodeForX(entityId);
    RemoveNodeForY(entityId);

   
    moveEntity->x = newX;
    moveEntity->y = newY;
    auto newGridPos = BaseAOIManager::GetGridCoord(newX, newY);
    moveEntity->gridX = newGridPos.first;
    moveEntity->gridY = newGridPos.second; 

    moveEntity->prevX = nullptr;
    moveEntity->nextX = nullptr;
    moveEntity->prevY = nullptr;
    moveEntity->nextY = nullptr;

    InsertNodeForX(moveEntity);
    InsertNodeForY(moveEntity);

    if(msgNotifyer_)
    {
        if(oldGridPos == newGridPos)
        {
            msgNotifyer_->MovePostionToMsgNotify(entityId, newX, newY, oldNeighborsEntitys);
        }
        else{
            auto newNeighborsEntitys = this->GetNeighbors(entityId);
            msgNotifyer_->MovePositionToMsgNotifyForGridChange(entityId, newX, newY, oldNeighborsEntitys, newNeighborsEntitys);
        }
    }
    
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
            if(!IsInRangeForGridPosition(node->gridX, node->gridY, preNode->gridX, preNode->gridY, radius))
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
            if(!IsInRangeForGridPosition(node->gridX, node->gridY, nextNode->gridX, nextNode->gridY, radius))
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
            if(!IsInRangeForGridPosition(node->gridX, node->gridY, preNode->gridX, preNode->gridY, radius))
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
            if(!IsInRangeForGridPosition(node->gridX, node->gridY, nextNode->gridX, nextNode->gridY, radius))
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


bool CrossListAOI::InsertNodeForX(std::shared_ptr<CrossListNode> node)
{
    // 处理x轴的情况
    if(nullptr == headX_)
    {
        headX_ = node;
        return true;
    }

    // 如果headX的本身就大于node的X值
    if((headX_->x) > (node->x))
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

GridCoordResult CrossListAOI::GetGridPosition(int entityId) const 
{
    GridCoordResult result;
    result.valid = false;
    result.gridX = 0;
    result.gridY = 0;
    auto itr = nodeMap_.find(entityId);
    if(itr == nodeMap_.end()) 
    {
        return result;
    }

    const auto& entity = (itr->second);
    result.valid = true;
    result.gridX = entity->gridX;
    result.gridY = entity->gridY;
    return result;
}

EntityPositionResult CrossListAOI::GetEntityPosition(int entityId) const 
{
    EntityPositionResult result;
    result.valid = false;
    result.x = 0;
    result.y = 0;
    auto itr = nodeMap_.find(entityId);
    if(itr == nodeMap_.end()) 
    {
        return result;
    }

    const auto& entity = (itr->second);
    result.valid = true;
    result.x = entity->x;
    result.y = entity->y;
    return result;
}