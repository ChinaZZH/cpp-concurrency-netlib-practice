#include "AOIManager.h"
#include <iostream>
#include <sstream>
#include "../../Common/LogFile.h"

 void AOIManager::AddEntity(int entityId, int x, int y)
 {
    // 支持坐标为负数
    /*
    if(!this->CheckPositionIsValid(x, y, "AOIManager::AddEntity"))
    {
        return;
    }
    */

    auto itr = entityMap_.find(entityId);
    if(itr != entityMap_.end())
    {
        std::cout << "AOIManager::AddEntity error entityID:=" << entityId << std::endl; 
        return ;
    }

    auto gridPostion = this->GetGridCoord(x, y);
    EntityInfo entity;
    entity.x = x;
    entity.y = y;
    entity.gridX = gridPostion.first;
    entity.gridY = gridPostion.second;
    entityMap_[entityId] = entity;

    //std::cout << "add entity map" << std::endl;
    
    auto& setEntityId = gridMap_[gridPostion];
    setEntityId.insert(entityId);

    // 同时进行九宫格广播加入的消息
    auto neighborsEntityList = this->GetNeighbors(entityId);
    for(const auto& neighborsId : neighborsEntityList)
    {
        // 广播有新entity消息下发，发送entityId, x, y
    }

 }


void AOIManager::RemoveEntity(int entityId)
{
    auto itrEntity = entityMap_.find(entityId);
    if(itrEntity == entityMap_.end())
    {
        std::cout << "AOIManager::RemoveEntity error entityID:=" << entityId << std::endl; 
        return ;
    }

    // 同时进行九宫格广播离开的消息
    auto neighborsEntityList = this->GetNeighbors(entityId);
    for(const auto& neighborsId : neighborsEntityList)
    {
        // 广播有删除entity消息下发，发送entityId, x, y
    }

    const EntityInfo& entity = (itrEntity->second);
    std::pair<int, int> gridPostion(entity.gridX, entity.gridY);
    entityMap_.erase(itrEntity);


    auto itrGrid = gridMap_.find(gridPostion);
    if(itrGrid != gridMap_.end())
    {
        auto& setEntityId = (itrGrid->second);
        setEntityId.erase(entityId);

        if(setEntityId.empty())
        {
            gridMap_.erase(gridPostion);
        }
    }
}


void AOIManager::MoveEntity(int entityId, int newX, int newY)
{
    // 支持坐标为负数
    /*
    if(!this->CheckPositionIsValid(newX, newY, "AOIManager::MoveEntity"))
    {
        return;
    }
    */

    // 移动entity的前提是 原先entityId就已经在了。
    auto itrEntity = entityMap_.find(entityId);
    if(itrEntity == entityMap_.end())
    {
        std::cout << "AOIManager::MoveEntity error entityID:=" << entityId << std::endl; 
        return ;
    }

    // 位置没有发生变化的时候，直接返回
    EntityInfo& entityInfo = (itrEntity->second);
    if(entityInfo.x == newX && entityInfo.y == newY)
    {
        return;
    }

    // 网格坐标没有发生变化则进行九宫格同步坐标变化即可
    auto oldGridPos = std::pair(entityInfo.gridX, entityInfo.gridY);
    auto oldNeighborsEntitys = this->GetNeighbors(entityId);


    auto newGridPos = this->GetGridCoord(newX, newY);

    // 更新entityInfo
    entityInfo.x = newX;
    entityInfo.y = newY;
    if(oldGridPos == newGridPos)
    {
        for(const auto& neighborsId : oldNeighborsEntitys)
        {
            // 广播entity 实体坐标 x, y 发生变化，发送entityId, x, y
        }

        return;
    }

    // 网格坐标发生变化，则需要先从旧的网格列表中删除该实体id
    entityInfo.gridX = newGridPos.first;
    entityInfo.gridY = newGridPos.second;
    auto itr_old_grid = gridMap_.find(oldGridPos);
    if(itr_old_grid != gridMap_.end())
    {
        auto& setOldEntityId = (itr_old_grid->second);
        setOldEntityId.erase(entityId);

        if(setOldEntityId.empty())
        {
            gridMap_.erase(oldGridPos);
        }
    }

    auto& setNewEntityId = gridMap_[newGridPos];
    setNewEntityId.insert(entityId);
    auto newNeighborsEntitys = this->GetNeighbors(entityId);

    // 假设add， remove， change坐标 发的是不同的消息，则需要先算出old和new的交集
    std::vector<int> vecAllNeighbors;
    std::set_intersection(oldNeighborsEntitys.begin(), oldNeighborsEntitys.end(), newNeighborsEntitys.begin(), newNeighborsEntitys.end(), back_inserter(vecAllNeighbors));
    for(const auto& allEntityId : vecAllNeighbors)
    {
        // 网格坐标变化前和变化后都在九宫格内, 则发送change positon消息
    }

    std::vector<int> vecNewNeighbors;
    std::set_intersection(newNeighborsEntitys.begin(), newNeighborsEntitys.end(), vecAllNeighbors.begin(), vecAllNeighbors.end(), back_inserter(vecNewNeighbors));
    for(const auto& EntityId : vecNewNeighbors)
    {
        // 网格坐标变化前不在九宫格内， 变化后再九宫格内，则发送新增消息
    }

    std::vector<int> vecRemoveNeighbors;
    std::set_intersection(oldNeighborsEntitys.begin(), oldNeighborsEntitys.end(), vecAllNeighbors.begin(), vecAllNeighbors.end(), back_inserter(vecRemoveNeighbors));
    for(const auto& EntityId : vecRemoveNeighbors)
    {
        // 网格坐标变化在九宫格内， 变化后不在九宫格内，则发送移除消息
    }
}
    
// 获取周围实体列表（默认九宫格，radius 可扩展）
std::vector<int> AOIManager::GetNeighbors(int entityId, int radius /*= 1*/) const
{
    std::vector<int> vecNeighbors;
    auto entityGridPos = this->GetGridPostion(entityId);
    if(false == entityGridPos.first) 
    {
        std::stringstream ss;
        ss << "AOIManager::GetNeighbors Get entity position error entityid: id" << entityId << std::endl;
        LogFile& logfile = LogFile::getInstance();
        logfile.AppendContent("AOIManager.txt", ss.str());

        return vecNeighbors;
    }

    // 支持坐标为负数
    auto pairGridPos = entityGridPos.second;
    int nMinX = pairGridPos.first - radius;
    int nMaxX = pairGridPos.first + radius;

    int nMinY = pairGridPos.second - radius;
    int nMaxY = pairGridPos.second + radius;

    for(int gridX = nMinX; gridX <= nMaxX; gridX += 1)
    {
        for(int gridY = nMinY; gridY <= nMaxY; gridY += 1)
        {
            auto neighborGridPos = std::pair(gridX, gridY);
            auto itr = gridMap_.find(neighborGridPos);
            if(itr == gridMap_.end())
            {
                continue;
            }

           
            const std::unordered_set<int>& setEntityIds = (itr->second);
            if(pairGridPos == neighborGridPos)
            {
                std::copy_if(setEntityIds.begin(), setEntityIds.end(), std::back_inserter(vecNeighbors), [entityId](int id){ return id != entityId; });
            }else{
                std::copy(setEntityIds.begin(), setEntityIds.end(), std::back_inserter(vecNeighbors));
            }
        }
    }

    return vecNeighbors;
}

// 辅助，根据实体坐标转换为网格坐标(物理坐标)
std::pair<bool,std::pair<int, int>> AOIManager::GetGridPostion(int entityId) const
{
    auto itr = entityMap_.find(entityId);
    if(itr == entityMap_.end()) 
    {
        return std::pair(false, std::pair(0, 0));
    }

    const EntityInfo& entity = (itr->second);
    return std::pair(true, std::pair(entity.gridX, entity.gridY));
}


std::pair<int, int> AOIManager::GetGridCoord(int x, int y) const
{
    return std::pair(x / gridSize_, y / gridSize_);
}

bool AOIManager::CheckPositionIsValid(int x, int y, const std::string& strLogContent)
{
    if(x < 0 || y < 0)
    {
        std::stringstream ss;
        ss << "x, y position error x:=" << x << " y:=" << y << " by call func:="<<strLogContent << std::endl;

        LogFile& logfile = LogFile::getInstance();
        logfile.AppendContent("AOIManager.txt", ss.str());
        return false;
    }

    return true;
}