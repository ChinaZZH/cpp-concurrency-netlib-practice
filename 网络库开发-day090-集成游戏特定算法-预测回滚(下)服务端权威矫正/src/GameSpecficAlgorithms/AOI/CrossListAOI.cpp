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
    newNode->info.lastUpdateTime = std::chrono::steady_clock::now();
    newNode->info.threadIdx = threadIdx_;
    newNode->info.parititionedPool = parititionedPool_;
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

   std::vector<int> neighborsEntityList;
   if(msgNotifyer_)
   {
        neighborsEntityList = this->GetNeighbors(entityId);
   }
   
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
    if(moveEntity->info.x == newX && moveEntity->info.y == newY)
    {
        return true;
    }

    // 或者新的坐标值 也是需要实体待在当前位置上的。
    bool changePosForX = true;
    if(moveEntity->info.x == newX)
    {
        changePosForX = false;
    }else {
        std::shared_ptr<CrossListNode> preX = moveEntity->prevX;
        std::shared_ptr<CrossListNode> nextX = moveEntity->nextX;
        if(((nullptr == preX) || (preX && preX->info.x <= newX)) && (nullptr == nextX) || (nextX && nextX->info.x >= newX))
        {
            changePosForX = false;
        }
    }

    bool changePosForY = true;
    if(moveEntity->info.y == newY)
    {
        changePosForY = false;
    }else {
        std::shared_ptr<CrossListNode> preY = moveEntity->prevY;
        std::shared_ptr<CrossListNode> nextY = moveEntity->nextY;
        if(((nullptr == preY) || (preY && preY->info.y <= newY)) && (nullptr == nextY) || (nextY && nextY->info.y >= newY))
        {
            changePosForY = false;
        }
    }


    // 玩家还是在那个位置上，则不进行更改，直接修改位置然后做消息同步即可
    std::vector<int> oldNeighborsEntitys;
    if(msgNotifyer_)
    {
        oldNeighborsEntitys = this->GetNeighbors(entityId);
    }

   
    // x 和 y 的链表位置都不用修改，则直接更改x,y坐标值然后视野内同步
    if(false == changePosForX && false == changePosForY)
    {
        std::pair<int, int> pairGridPos = BaseAOIManager::GetGridCoord(newX, newY);
        moveEntity->info.x = newX;
        moveEntity->info.y = newY;
        moveEntity->info.gridX = pairGridPos.first;
        moveEntity->info.gridY = pairGridPos.second;
        moveEntity->info.lastUpdateTime = std::chrono::steady_clock::now();
        
        
        if(msgNotifyer_)
        {
            ProcessMoveMessage(entityId, moveEntity->info, oldNeighborsEntitys);
        }
        
        return true;
    }

    
    std::pair<int, int> oldGridPos(moveEntity->info.gridX, moveEntity->info.gridY);
    auto newGridPos = BaseAOIManager::GetGridCoord(newX, newY);

    // 需要删除节点的情况下，判断是从头开始遍历还是当前节点开始遍历 
    // x轴的情况
    if(changePosForX)
    {
         /*
        RemoveNodeForX(entityId);
        moveEntity->info.x = newX;
        moveEntity->info.gridX = newGridPos.first;
        InsertNodeForX(moveEntity);
        */

        // 最开始使用删除，插入，后续优化成MoveNodeToNewX
        MoveNodeToNewX(entityId, newX, newGridPos.first);
    }
    

    if(changePosForY)
    {
        /*
        RemoveNodeForY(entityId);
        moveEntity->info.y = newY;
        moveEntity->info.gridY = newGridPos.second;
        InsertNodeForY(moveEntity);
        */

        // 最开始使用删除，插入，后续优化成MoveNodeToNewY
        MoveNodeToNewY(entityId, newY, newGridPos.second);
    }

    moveEntity->info.lastUpdateTime = std::chrono::steady_clock::now();
    if(msgNotifyer_)
    {
        ProcessMoveMessage(entityId, moveEntity->info, oldNeighborsEntitys);
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
            if(!IsInRangeForGridPosition(node->info.gridX, node->info.gridY, preNode->info.gridX, preNode->info.gridY, radius))
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
            if(!IsInRangeForGridPosition(node->info.gridX, node->info.gridY, nextNode->info.gridX, nextNode->info.gridY, radius))
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
            if(!IsInRangeForGridPosition(node->info.gridX, node->info.gridY, preNode->info.gridX, preNode->info.gridY, radius))
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
            if(!IsInRangeForGridPosition(node->info.gridX, node->info.gridY, nextNode->info.gridX, nextNode->info.gridY, radius))
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
    if((headX_->info.x) > (node->info.x))
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
        // 当前nodeNext的X数值比node->info.x的更大，则node需要挂在nodeNextX的前面
        if((nodeNextX->info.x) > (node->info.x))
        {
            break;
        }

        // 下面逻辑是nodeNext的X数值node->info.x的更小的逻辑，如果已经没有办法往后走了，说明当前已经是最大值了，则break，将node挂在最后面
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
    if((headY_->info.y) > (node->info.y))
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
        // 当前nodeNext的y数值比node->info.y的更大，则node需要挂在nodeNext的前面
        if((nodeNextY->info.y) > (node->info.y))
        {
            break;
        }

        // 下面逻辑是nodeNext的y数值node->info.y的更小的逻辑，如果已经没有办法往后走了，说明当前已经是最大值了，将新结点挂在node挂在最后面.
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
    auto itr = nodeMap_.find(entityId);
    if (itr == nodeMap_.end())
    {
        return false;
    }

    std::shared_ptr<CrossListNode> removeNode = (itr->second);
    std::shared_ptr<CrossListNode> preNode =  removeNode->prevX;
    std::shared_ptr<CrossListNode> nextNode = removeNode->nextX;
    if(nullptr == preNode && nullptr == nextNode)
    {
        headX_ = nullptr;
        removeNode->prevX = nullptr;
        removeNode->nextX = nullptr;
        return true;
    }

    if(removeNode == headX_){
        headX_ = nextNode;
    }

    if(preNode)
    {
        preNode->nextX = nextNode;
    }

    if(nextNode)
    {
        nextNode->prevX = preNode;
    }

    removeNode->prevX = nullptr;
    removeNode->nextX = nullptr;

    /*
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
    
    if(!preNode)
    {
        return false;
    }

    
    preNode->nextX = nextNode;
    if(nextNode)
    {
        nextNode->prevX = preNode;
    } 

    // 将findNode的连接清空
    findNode->prevX = nullptr;
    findNode->nextX = nullptr;
    */
    return true; 
}


bool CrossListAOI::RemoveNodeForY(int entityId)
{
    /*
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
    */

    auto itr = nodeMap_.find(entityId);
    if (itr == nodeMap_.end())
    {
        return false;
    }

    std::shared_ptr<CrossListNode> removeNode = (itr->second);
    std::shared_ptr<CrossListNode> preNode =  removeNode->prevY;
    std::shared_ptr<CrossListNode> nextNode = removeNode->nextY;
    if(nullptr == preNode && nullptr == nextNode)
    {
        headY_ = nullptr;
        removeNode->prevY = nullptr;
        removeNode->nextY = nullptr;
        return true;
    }

    if(removeNode == headY_)
    {
        headY_ = nextNode;
    }

    if(preNode)
    {
        preNode->nextY = nextNode;
    } 

    if(nextNode)
    {
        nextNode->prevY = preNode;
    }

    removeNode->prevY = nullptr;
    removeNode->nextY = nullptr;
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
    result.gridX = entity->info.gridX;
    result.gridY = entity->info.gridY;
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
    result.x = entity->info.x;
    result.y = entity->info.y;
    result.lastUpdateTime = entity->info.lastUpdateTime;
    return result;
}


bool CrossListAOI::MoveNodeToNewX(int entityId, int newX, int newGridX)
{
    auto itr = nodeMap_.find(entityId);
    if(itr == nodeMap_.end())
    {
        return false;
    }

    auto moveNode = (itr->second);
    if(moveNode->info.x == newX)
    {
        return true;
    }

     // 如果只有一个节点并且是head结点, 说明当前节点就是头结点，只要设置x,y的数值即可
    if(nullptr == headX_->nextX)
    {
        moveNode->info.x = newX;
        moveNode->info.gridX = newGridX;
        return true;
    }

    
    
    // 统一都是从前往后找，不做从后往前找的优化
    std::shared_ptr<CrossListNode> newNextNode;
    if(newX > (moveNode->info.x))
    {
        if(nullptr == moveNode->nextX)
        {
            moveNode->info.x = newX;
            moveNode->info.gridX = newGridX;
            return true;
        }

        newNextNode = moveNode->nextX;
        RemoveNodeForX(entityId); // 必须取到next之后删除，不然取不到next
    }
    else
    {
        RemoveNodeForX(entityId);
        newNextNode = headX_;  // 删除完entityId，取到的headX才是最真实的headX.
        // 逻辑错误需要排查
        if(nullptr == newNextNode)
        {
            std::cout << "CrossListAOI::MoveNodeToNewX findNode nullptr Server Logic Error " << std::endl;
        }
    }

    moveNode->info.x = newX;
    moveNode->info.gridX = newGridX;
    while(newNextNode)
    {
        // 当前nodeNext的X数值比node->info.x的更大，则node需要挂在nodeNextX的前面
        if((newNextNode->info.x) > newX)
        {
            break;
        }

        
        if(newNextNode->nextX)
        {
           newNextNode = newNextNode->nextX;
           continue;
        }        

        // 下面逻辑是nodeNext的X数值node->info.x的更小的逻辑，如果已经没有办法往后走了，说明当前已经是最大值了，则break，将node挂在最后面
        newNextNode->nextX = moveNode;
        moveNode->prevX = newNextNode;
        return true;
    }

    // 逻辑错误需要排查
    if(nullptr == newNextNode)
    {
        std::cout << "CrossListAOI::MoveNodeToNewX findNode nullptr Server Logic Error 2222" << std::endl;
        return false;
    }

    std::shared_ptr<CrossListNode> preX = newNextNode->prevX;
    if(preX)
    {
        preX->nextX = moveNode;
        moveNode->prevX = preX;
    }
    else{
        headX_ = moveNode;
    }

    moveNode->nextX = newNextNode;
    newNextNode->prevX = moveNode;
    return true;
}

bool CrossListAOI::MoveNodeToNewY(int entityId, int newY, int newGridY)
{
    auto itr = nodeMap_.find(entityId);
    if(itr == nodeMap_.end())
    {
        return false;
    }

    auto moveNode = (itr->second);
    if(moveNode->info.y == newY)
    {
        return true;
    }

     // 如果只有一个节点并且是head结点, 说明当前节点就是头结点，只要设置x,y的数值即可
    if(nullptr == headY_->nextY)
    {
        moveNode->info.y = newY;
        moveNode->info.gridY = newGridY;
        return true;
    }

    
    
    // 统一都是从前往后找，不做从后往前找的优化
    std::shared_ptr<CrossListNode> newNextNode;
    if(newY > (moveNode->info.y))
    {
        if(nullptr == moveNode->nextY)
        {
            moveNode->info.y = newY;
            moveNode->info.gridY = newGridY;
            return true;
        }

        newNextNode = moveNode->nextY;
        RemoveNodeForY(entityId); // 必须取到next之后删除，不然取不到next
    }
    else
    {
        RemoveNodeForY(entityId);
        newNextNode = headY_;  // 删除完entityId，取到的headX才是最真实的headX.
        // 逻辑错误需要排查
        if(nullptr == newNextNode)
        {
            std::cout << "CrossListAOI::MoveNodeToNewY findNode nullptr Server Logic Error " << std::endl;
        }
    }

    moveNode->info.y = newY;
    moveNode->info.gridY = newGridY;
    while(newNextNode)
    {
        // 当前nodeNext的X数值比node->info.x的更大，则node需要挂在nodeNextX的前面
        if((newNextNode->info.y) > newY)
        {
            break;
        }

        
        if(newNextNode->nextY)
        {
           newNextNode = newNextNode->nextY;
           continue;
        }        

        // 下面逻辑是nodeNext的X数值node->info.x的更小的逻辑，如果已经没有办法往后走了，说明当前已经是最大值了，则break，将node挂在最后面
        newNextNode->nextY = moveNode;
        moveNode->prevY = newNextNode;
        return true;
    }

    // 逻辑错误需要排查
    if(nullptr == newNextNode)
    {
        std::cout << "CrossListAOI::MoveNodeToNewY findNode nullptr Server Logic Error 2222" << std::endl;
        return false;
    }

    std::shared_ptr<CrossListNode> preY = newNextNode->prevY;
    if(preY)
    {
        preY->nextY = moveNode;
        moveNode->prevY = preY;
    }
    else{
        headY_ = moveNode;
    }

    moveNode->nextY = newNextNode;
    newNextNode->prevY = moveNode;
    return true;
}