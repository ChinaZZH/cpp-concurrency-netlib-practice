#include "AOIManager.h"
#include <iostream>
#include <sstream>
#include "../../Common/LogFile.h"
#include "../../../build/proto_gen/aoi.pb.h"

 bool AOIManager::AddEntity(int entityId, int x, int y)
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
        std::stringstream ss;
        ss << "AOIManager::AddEntity error entityID:=" << entityId << std::endl; 
        LogFile& logfile = LogFile::getInstance();
        logfile.AppendContent("AOIManager.txt", ss.str());

        std::cout << "AOIManager::AddEntity error entityID:=" << entityId << std::endl; 
        return false;
    }

    // 存储数据修改
    EntityInfo entity;
    entity.x = x;
    entity.y = y;
    auto gridPostion = this->GetGridCoord(x, y);
    entity.gridX = gridPostion.first;
    entity.gridY = gridPostion.second;

    {
        entityMap_[entityId] = entity;
        auto& setEntityId = gridMap_[gridPostion];
        setEntityId.insert(entityId);
    }
    

    // 广播有新entity消息下发newEntityResponse，发送entityId, x, y
    auto neighborsEntityList = this->GetNeighbors(entityId);
    EnterNewGridPosMsgNotify(entityId, entity, neighborsEntityList);
    return true;
 }


bool AOIManager::RemoveEntity(int entityId)
{
    auto itrEntity = entityMap_.find(entityId);
    if(itrEntity == entityMap_.end())
    {
        std::stringstream ss;
        ss << "AOIManager::RemoveEntity error entityID:=" << entityId << std::endl;
        LogFile& logfile = LogFile::getInstance();
        logfile.AppendContent("AOIManager.txt", ss.str());

        std::cout << "AOIManager::RemoveEntity error entityID:=" << entityId << std::endl; 
        return false;
    }

    auto neighborsEntityList = this->GetNeighbors(entityId);
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
    

    // 消息同步
    aoi::EntityLeaveNotifyResponse entityLeaveResponse;
    entityLeaveResponse.set_entity_id(entityId);
    
    std::string strData;
    entityLeaveResponse.SerializeToString(&strData);
    BroadEntityList(neighborsEntityList, strData, GSMT_RemoveEntity);
    return true;
}


bool AOIManager::MoveEntity(int entityId, int newX, int newY)
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
        std::stringstream ss;
        ss << "AOIManager::MoveEntity error entityID:=" << entityId << std::endl;
        LogFile& logfile = LogFile::getInstance();
        logfile.AppendContent("AOIManager.txt", ss.str());

        std::cout << "AOIManager::MoveEntity error entityID:=" << entityId << std::endl; 
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
    auto oldNeighborsEntitys = this->GetNeighbors(entityId);

    // 更新entityInfo
    entityInfo.x = newX;
    entityInfo.y = newY;
    auto newGridPos = this->GetGridCoord(newX, newY);

    // 实际物理坐标改变，但是网格坐标没有改变，则向邻居同步最新的坐标
    if(oldGridPos == newGridPos)
    {
        aoi::EntityMoveNotifyResponse moveResponse;
        moveResponse.set_entity_id(entityId);
        moveResponse.set_new_x(newX);
        moveResponse.set_new_y(newY);
        
        std::string strData;
        moveResponse.SerializeToString(&strData);
        BroadEntityList(oldNeighborsEntitys, strData, GSMT_MoveEntity);
        return true;
    }

    // 网格坐标发生变化，则需要先从旧的网格列表中删除该实体id
    // 存储数据修改
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
    
    auto newNeighborsEntitys = this->GetNeighbors(entityId);
    std::vector<int> vecAllNeighbors;
    std::set_intersection(oldNeighborsEntitys.begin(), oldNeighborsEntitys.end(), newNeighborsEntitys.begin(), newNeighborsEntitys.end(), back_inserter(vecAllNeighbors));

    // 网格坐标变化前和变化后都在entity视野的九宫格内, 则发送change positon消息
    {
        aoi::EntityMoveNotifyResponse moveResponse;
        moveResponse.set_entity_id(entityId);
        moveResponse.set_new_x(newX);
        moveResponse.set_new_y(newY);

        std::string strData;
        moveResponse.SerializeToString(&strData);
        BroadEntityList(vecAllNeighbors, strData, GSMT_MoveEntity);
    }

    
    // 离开玩家九宫格视野的人, 广播离开的消息
    {
        aoi::EntityLeaveNotifyResponse entityLeaveResponse;
        entityLeaveResponse.set_entity_id(entityId);
        std::string strData;
        entityLeaveResponse.SerializeToString(&strData);

        std::vector<int> vecRemoveNeighbors;
        std::set_difference(oldNeighborsEntitys.begin(), oldNeighborsEntitys.end(), vecAllNeighbors.begin(), vecAllNeighbors.end(), back_inserter(vecRemoveNeighbors));
        BroadEntityList(vecRemoveNeighbors, strData, GSMT_RemoveEntity);
    }
    
    
    // 处理新进入视野的人
    std::vector<int> vecAddNewNeighbors;
    std::set_difference(newNeighborsEntitys.begin(), newNeighborsEntitys.end(), vecAllNeighbors.begin(), vecAllNeighbors.end(), back_inserter(vecAddNewNeighbors));
    EnterNewGridPosMsgNotify(entityId, entityInfo, vecAddNewNeighbors);
    return true;
}
    

void AOIManager::EnterNewGridPosMsgNotify(int entityId, const EntityInfo& newEntity, const std::vector<int>& neighborsEntityList)
{
    {
        aoi::EntityEnterNotifyResponse newEntityResponse;
        aoi::EntityInfo* pNewEntity = newEntityResponse.mutable_new_entity();
        if(pNewEntity)
        {
            pNewEntity->set_entity_id(entityId);
            pNewEntity->set_x(newEntity.x);
            pNewEntity->set_y(newEntity.y);
        }

        std::string strData;
        newEntityResponse.SerializeToString(&strData);
        BroadEntityList(neighborsEntityList, strData, GSMT_AddEntity);
    }
    


    // 同时进行九宫格广播有新的人进入视野的消息给邻居
    aoi::InitAroundEntitiesNotifyResponse aroundResponse;
    for(const auto& neighborsId : neighborsEntityList)
    {
       // 将neighborsId 压入到数据包中，然后结束后发给新进入者
       auto neighborsInfo = this->GetEntityPostion(neighborsId);
       if(!neighborsInfo.valid)
       {
            continue;
       }
       
       aoi::EntityInfo* pAroundEntity =  aroundResponse.add_around_entities();
       if(!pAroundEntity)
       {
            continue;
       }
       

       pAroundEntity->set_entity_id(neighborsId);
       pAroundEntity->set_x(neighborsInfo.x);
       pAroundEntity->set_y(neighborsInfo.y);
    }

    // 将aroundResponse发送给进入新进入者
    if(sendMsgCallBack_)
    {
        std::string strData;
        aroundResponse.SerializeToString(&strData);
        sendMsgCallBack_(entityId, strData, GSMT_SyncNeighborsEntity);
    }   
}


// 获取周围实体列表（默认九宫格，radius 可扩展）
std::vector<int> AOIManager::GetNeighbors(int entityId, int radius /*= 1*/) const
{
    std::vector<int> vecNeighbors;
    auto entityGridPos = this->GetGridPostion(entityId);
    if(false == entityGridPos.valid) 
    {
        std::stringstream ss;
        ss << "AOIManager::GetNeighbors Get entity position error entityid: id" << entityId << std::endl;
        LogFile& logfile = LogFile::getInstance();
        logfile.AppendContent("AOIManager.txt", ss.str());

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
GridCoordResult AOIManager::GetGridPostion(int entityId) const
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


EntityPositionResult AOIManager::GetEntityPostion(int entityId) const
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
    return result;
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


void AOIManager::BroadNeighborsEntityList(int entityId, const std::string& strMsg, GameServerMsgType msgType)
{
    auto neighborsEntityList = this->GetNeighbors(entityId);
    BroadEntityList(neighborsEntityList, strMsg, msgType);
}

void AOIManager::BroadEntityList(const std::vector<int>& entityList, const std::string& strMsg, GameServerMsgType msgType)
{
    for(const auto& neighborsId : entityList)
    {
        if(sendMsgCallBack_)
        {
            sendMsgCallBack_(neighborsId, strMsg, msgType);
        }
    }
}