#include "GridAOI.h"
#include <iostream>
#include <sstream>
#include "../../Common/LogFile.h"
#include "../../../build/proto_gen/aoi.pb.h"

 bool GridAOI::AddEntity(int entityId, int x, int y)
 {
    auto itr = entityMap_.find(entityId);
    if(itr != entityMap_.end())
    {
        std::stringstream ss;
        ss << "GridAOI::AddEntity error entityID:=" << entityId << std::endl; 
        LogFile& logfile = LogFile::getInstance();
        logfile.AppendContent("GridAOI.txt", ss.str());

        std::cout << "GridAOI::AddEntity error entityID:=" << entityId << std::endl; 
        return false;
    }

    // 存储数据修改
    EntityInfo entity;
    entity.threadIdx = threadIdx_;
    entity.parititionedPool = parititionedPool_;
    entity.x = x;
    entity.y = y;
    auto gridPostion = this->GetGridCoord(x, y);
    entity.gridX = gridPostion.first;
    entity.gridY = gridPostion.second;
    entity.lastUpdateTime = std::chrono::steady_clock::now();
    entity.broadcastX = x;
    entity.broadcastY = y;
    entity.timerId = 0;

    {
        entityMap_[entityId] = entity;
        auto& setEntityId = gridMap_[gridPostion];
        setEntityId.insert(entityId);
    }
    

    // 广播有新entity消息下发newEntityResponse，发送entityId, x, y
    if(msgNotifyer_)
    {
        auto neighborsEntityList = this->GetNeighbors(entityId);
        msgNotifyer_->EnterNewGridPosMsgNotify(entityId, x, y, neighborsEntityList);
    }
    
    return true;
 }


bool GridAOI::RemoveEntity(int entityId)
{
    auto itrEntity = entityMap_.find(entityId);
    if(itrEntity == entityMap_.end())
    {
        std::stringstream ss;
        ss << "GridAOI::RemoveEntity error entityID:=" << entityId << std::endl;
        LogFile& logfile = LogFile::getInstance();
        logfile.AppendContent("GridAOI.txt", ss.str());

        std::cout << "GridAOI::RemoveEntity error entityID:=" << entityId << std::endl; 
        return false;
    }

    std::vector<int> neighborsEntityList;
    if(msgNotifyer_)
    {
        neighborsEntityList = this->GetNeighbors(entityId);
    }
     
    // 存储数据修改
    {
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
    

    if(msgNotifyer_)
    {
        msgNotifyer_->LeaveGridToMsgNotify(entityId, neighborsEntityList);
    }

    return true;
}


bool GridAOI::MoveEntity(int entityId, int newX, int newY)
{
    // 移动entity的前提是 原先entityId就已经在了。
    auto itrEntity = entityMap_.find(entityId);
    if(itrEntity == entityMap_.end())
    {
        std::stringstream ss;
        ss << "GridAOI::MoveEntity error entityID:=" << entityId << std::endl;
        LogFile& logfile = LogFile::getInstance();
        logfile.AppendContent("GridAOI.txt", ss.str());

        std::cout << "GridAOI::MoveEntity error entityID:=" << entityId << std::endl; 
        return false;
    }

    // 位置没有发生变化的时候，直接返回
    EntityInfo& entityInfo = (itrEntity->second);
    if(entityInfo.x == newX && entityInfo.y == newY)
    {
        return true;
    }

    // 网格坐标没有发生变化则进行九宫格同步坐标变化即可
    auto oldGridPos = std::pair(entityInfo.gridX, entityInfo.gridY);
    std::vector<int> oldNeighborsEntitys;
    if(msgNotifyer_)
    {
       oldNeighborsEntitys = this->GetNeighbors(entityId);
    }
     

    // 更新entityInfo
    entityInfo.x = newX;
    entityInfo.y = newY;
    entityInfo.lastUpdateTime = std::chrono::steady_clock::now();

    auto newGridPos = this->GetGridCoord(newX, newY);

    // 网格坐标发生变化，则需要先从旧的网格列表中删除该实体id
    // 存储数据修改
    if(oldGridPos != newGridPos)
    {
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
    }
    
    if(msgNotifyer_)
    {
        ProcessMoveMessage(entityId, entityInfo, oldNeighborsEntitys);
    }

    return true;
}
    


// 获取周围实体列表（默认九宫格，radius 可扩展）
std::vector<int> GridAOI::GetNeighbors(int entityId, int radius /*= 1*/) const
{
    std::vector<int> vecNeighbors;
    auto entityGridPos = this->GetGridPosition(entityId);
    if(false == entityGridPos.valid) 
    {
        std::stringstream ss;
        ss << "GridAOI::GetNeighbors Get entity position error entityid: id" << entityId << std::endl;
        LogFile& logfile = LogFile::getInstance();
        logfile.AppendContent("GridAOI.txt", ss.str());

        return vecNeighbors;
    }

    // 支持坐标为负数
   // 支持坐标为负数
    int nMinX = entityGridPos.gridX - radius;
    int nMaxX = entityGridPos.gridX + radius;

    int nMinY = entityGridPos.gridY - radius;
    int nMaxY = entityGridPos.gridY + radius;

    for(int gridX = nMinX; gridX <= nMaxX; gridX += 1)
    {
        for(int gridY = nMinY; gridY <= nMaxY; gridY += 1)
        {
            //auto neighborGridPos = std::pair(gridX, gridY);
            auto itr = gridMap_.find(std::pair(gridX, gridY));
            if(itr == gridMap_.end())
            {
                continue;
            }

           
            const std::set<int>& setEntityIds = (itr->second);
            if(gridX == entityGridPos.gridX && gridY == entityGridPos.gridY)
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
GridCoordResult GridAOI::GetGridPosition(int entityId) const
{
    GridCoordResult result;
    result.valid = false;
    result.gridX = 0;
    result.gridY = 0;
    auto itr = entityMap_.find(entityId);
    if(itr == entityMap_.end()) 
    {
        return result;
    }

    const EntityInfo& entity = (itr->second);
    result.valid = true;
    result.gridX = entity.gridX;
    result.gridY = entity.gridY;
    return result;
}


EntityPositionResult GridAOI::GetEntityPosition(int entityId) const
{
    EntityPositionResult result;
    result.valid = false;
    result.x = 0;
    result.y = 0;
    auto itr = entityMap_.find(entityId);
    if(itr == entityMap_.end()) 
    {
        return result;
    }

    const EntityInfo& entity = (itr->second);
    result.valid = true;
    result.x = entity.x;
    result.y = entity.y;
    result.lastUpdateTime = entity.lastUpdateTime;
    return result;
}

