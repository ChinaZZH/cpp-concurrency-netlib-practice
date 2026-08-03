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
    if(newNeighborsEntitys.empty() || !sendMsgCallBack_)
    {
        return;
    }

    {
        aoi::EntityEnterNotifyResponse newEntityResponse;
        aoi::EntityInfo* pNewEntity = newEntityResponse.mutable_new_entity();
        if(pNewEntity)
        {
            pNewEntity->set_entity_id(entityId);
            pNewEntity->set_x(entityX);
            pNewEntity->set_y(entityY);
        }

        for (int recv_id : newNeighborsEntitys) 
        {
           newEntityResponse.set_msg_client_id(recv_id);
                
            std::string strData;
            newEntityResponse.SerializeToString(&strData);
            sendMsgCallBack_(entityId, strData, GSMT_AddEntity);
        }
    }
    


    // 同时进行九宫格广播有新的人进入视野的消息给邻居
    aoi::InitAroundEntitiesNotifyResponse aroundResponse;
    aroundResponse.set_msg_client_id(entityId);
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
    std::string strData;
    aroundResponse.SerializeToString(&strData);
    sendMsgCallBack_(entityId, strData, GSMT_SyncNeighborsEntity);
 }


 void AOIMsgNotifyer::LeaveGridToMsgNotify(int entityId, const std::vector<int>& neighborsEntitys)
 {
    if(neighborsEntitys.empty() || !sendMsgCallBack_)
    {
        return;
    }

    // 消息同步
    aoi::EntityLeaveNotifyResponse entityLeaveResponse;
    entityLeaveResponse.set_entity_id(entityId);
    
    for(int recv_id : neighborsEntitys) 
    {
        entityLeaveResponse.set_msg_client_id(recv_id);
                
        std::string strData;
        entityLeaveResponse.SerializeToString(&strData);
        sendMsgCallBack_(entityId, strData, GSMT_RemoveEntity);
    }   
 }


 void AOIMsgNotifyer::MovePostionToMsgNotify(int entityId, int newX, int newY, const std::vector<int>& neighborsEntitys)
 {
    if(neighborsEntitys.empty() || !sendMsgCallBack_)
    {
        return;
    }

    aoi::EntityMoveNotifyResponse moveResponse;
    moveResponse.set_entity_id(entityId);
    moveResponse.set_new_x(newX);
    moveResponse.set_new_y(newY);
        
    for(int recv_id : neighborsEntitys) 
    {
        moveResponse.set_msg_client_id(recv_id);
                
        std::string strData;
        moveResponse.SerializeToString(&strData);
        sendMsgCallBack_(entityId, strData, GSMT_MoveEntity);
    }   
 }


 void AOIMsgNotifyer::BroadcastMsgForMoveAction(int broadcastMask, int entityId, int entityX, int entityY, const std::vector<int>& oldNeighborsEntitys, const std::vector<int>& newNeighborsEntitys)
 {
    std::vector<int> vecKeepInSightNeighbors;
    std::set_intersection(oldNeighborsEntitys.begin(), oldNeighborsEntitys.end(), newNeighborsEntitys.begin(), newNeighborsEntitys.end(), back_inserter(vecKeepInSightNeighbors));

    // 网格坐标变化前和变化后都在entity视野的九宫格内, 则发送change positon消息
    // 移动距离超过阈值才同步
    if(broadcastMask & AOI_MOVE)
    {
        this->MovePostionToMsgNotify(entityId, entityX, entityY, vecKeepInSightNeighbors);
    }
    

    // 离开玩家九宫格视野的人, 广播离开的消息
    if(broadcastMask & AOI_DEL)
    {
        std::vector<int> vecRemoveNeighbors;
        std::set_difference(oldNeighborsEntitys.begin(), oldNeighborsEntitys.end(), vecKeepInSightNeighbors.begin(), vecKeepInSightNeighbors.end(), back_inserter(vecRemoveNeighbors));
        this->LeaveGridToMsgNotify(entityId, vecRemoveNeighbors);
    }
    
    
    // 处理新进入视野的人
    if(broadcastMask & AOI_ADD)
    {
        std::vector<int> vecAddNewNeighbors;
        std::set_difference(newNeighborsEntitys.begin(), newNeighborsEntitys.end(), vecKeepInSightNeighbors.begin(), vecKeepInSightNeighbors.end(), back_inserter(vecAddNewNeighbors));
        this->EnterNewGridPosMsgNotify(entityId, entityX, entityY, vecAddNewNeighbors);
    }
 }