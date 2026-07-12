#include "AOIMsgNotifyer.h"
#include <iostream>
#include <sstream>
#include "../../Common/LogFile.h"
#include "../../../build/proto_gen/aoi.pb.h"

void AOIMsgNotifyer::BroadcastToEntities(const std::vector<int>& entityList, const std::string& msg, GameServerMsgType msgType) {
    for (int entityId : entityList) 
    {
        if (sendMsgCallBack_) 
        {
            sendMsgCallBack_(entityId, msg, msgType);
        }
    }
}


 void AOIMsgNotifyer::EnterNewGridPosMsgNotify(int entityId, int entityX, int entityY, const std::vector<int>& newNeighborsEntitys)
 {
    {
        aoi::EntityEnterNotifyResponse newEntityResponse;
        aoi::EntityInfo* pNewEntity = newEntityResponse.mutable_new_entity();
        if(pNewEntity)
        {
            pNewEntity->set_entity_id(entityId);
            pNewEntity->set_x(entityX);
            pNewEntity->set_y(entityY);
        }

        std::string strData;
        newEntityResponse.SerializeToString(&strData);
        BroadcastToEntities(newNeighborsEntitys, strData, GSMT_AddEntity);
    }
    


    // 同时进行九宫格广播有新的人进入视野的消息给邻居
    aoi::InitAroundEntitiesNotifyResponse aroundResponse;
    for(const auto& neighborsId : newNeighborsEntitys)
    {
       // 将neighborsId 压入到数据包中，然后结束后发给新进入者
       auto neighborsInfo = aoiInterface_->GetEntityPosition(neighborsId);
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


 void AOIMsgNotifyer::LeaveGridToMsgNotify(int entityId, const std::vector<int>& neighborsEntitys)
 {
    // 消息同步
    aoi::EntityLeaveNotifyResponse entityLeaveResponse;
    entityLeaveResponse.set_entity_id(entityId);
    
    std::string strData;
    entityLeaveResponse.SerializeToString(&strData);
    this->BroadcastToEntities(neighborsEntitys, strData, GSMT_RemoveEntity);
 }


 void AOIMsgNotifyer::MovePostionToMsgNotify(int entityId, int newX, int newY, const std::vector<int>& neighborsEntitys)
 {
    aoi::EntityMoveNotifyResponse moveResponse;
    moveResponse.set_entity_id(entityId);
    moveResponse.set_new_x(newX);
    moveResponse.set_new_y(newY);
        
    std::string strData;
    moveResponse.SerializeToString(&strData);
    this->BroadcastToEntities(neighborsEntitys, strData, GSMT_MoveEntity);
 }


 void AOIMsgNotifyer::MovePositionToMsgNotifyForGridChange(int entityId, int entityX, int entityY, const std::vector<int>& oldNeighborsEntitys, const std::vector<int>& newNeighborsEntitys)
 {
     std::vector<int> vecAllNeighbors;
    std::set_intersection(oldNeighborsEntitys.begin(), oldNeighborsEntitys.end(), newNeighborsEntitys.begin(), newNeighborsEntitys.end(), back_inserter(vecAllNeighbors));

    // 网格坐标变化前和变化后都在entity视野的九宫格内, 则发送change positon消息
    this->MovePostionToMsgNotify(entityId, entityX, entityY, vecAllNeighbors);

    
    // 离开玩家九宫格视野的人, 广播离开的消息
    {
        std::vector<int> vecRemoveNeighbors;
        std::set_difference(oldNeighborsEntitys.begin(), oldNeighborsEntitys.end(), vecAllNeighbors.begin(), vecAllNeighbors.end(), back_inserter(vecRemoveNeighbors));
        this->LeaveGridToMsgNotify(entityId, vecRemoveNeighbors);
    }
    
    
    // 处理新进入视野的人
    {
        std::vector<int> vecAddNewNeighbors;
        std::set_difference(newNeighborsEntitys.begin(), newNeighborsEntitys.end(), vecAllNeighbors.begin(), vecAllNeighbors.end(), back_inserter(vecAddNewNeighbors));
        this->EnterNewGridPosMsgNotify(entityId, entityX, entityY, vecAddNewNeighbors);
    }
 }