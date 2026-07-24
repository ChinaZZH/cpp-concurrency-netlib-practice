#pragma once

#include "IAOIManager.h"
#include "AOIMsgNotifyer.h"
#include "../PartitionedPool.h"
#include <unordered_map>
#include <mutex>
#include <memory>
#include <cstdlib> 
#include <chrono>

// ============================================================
// 实体信息
// ============================================================
struct EntityInfo
{
    int x;
    int y;

    int gridX;
    int gridY;

    std::chrono::steady_clock::time_point lastUpdateTime;
    int broadcastX;
    int broadcastY;
    int timerId = 0;

    int threadIdx = 0;
    std::shared_ptr<PartitionedPool> parititionedPool;

    // 实现RAII
    ~EntityInfo()
    {
        if(timerId > 0 && parititionedPool)
        {
            parititionedPool->CancelTimer(threadIdx, timerId);
        }
    }
};

class BaseAOIManager : public IAOIManager {
public:
    explicit BaseAOIManager(int gridSize)
        : gridSize_(gridSize) {}

    virtual ~BaseAOIManager() = default;

    virtual void InitAoiData(int threadIdx, int moveThreshold, std::shared_ptr<PartitionedPool> parititionedPool, int forceMoveMsgDelaySeconds) override
    {
        threadIdx_ = threadIdx;
        squareMaxDis_ = moveThreshold * moveThreshold;
        parititionedPool_ = parititionedPool;
        delaySecondBroadcastMove_ = forceMoveMsgDelaySeconds;
    }


    // 公共方法：广播回调注入（子类可直接复用）
    virtual void SetSendMessageCallBack(SendMsgCallBack cb) override {
        //sendMsgCallBack_ = cb;
        msgNotifyer_ = std::make_unique<AOIMsgNotifyer>(cb, this);
    }

   

    virtual std::pair<int, int> GetGridCoord(int x, int y) const override {
        return std::pair(x / gridSize_, y / gridSize_);
    }

     virtual bool IsInRangeForGridPosition(int gridX1, int gridY1, int gridX2, int gridY2, int radius) const override {
        return std::abs(gridX1 - gridX2) <= radius && std::abs(gridY1 - gridY2) <= radius;
     }

     virtual bool IsInRange(int x1, int y1, int x2, int y2, int radius) const override {
        std::pair<int, int> prePos = this->GetGridCoord(x1, y1);
        std::pair<int, int> nextPos = this->GetGridCoord(x2, y2);
        return this->IsInRangeForGridPosition(prePos.first, prePos.second, nextPos.first, nextPos.second, radius);
     }


     virtual bool IsBroadcastMoveMessage(int x1, int y1, int x2, int y2) override {
        int deltaX = x1 - x2;
        int deltaY = y1 - y2;
        int squareDistance = deltaX * deltaX + deltaY * deltaY;
        /*
        if(squareDistance <= squareMaxDis_)
        {
            std::cout << "IsBroadcastMoveMessage broadcast failed" << std::endl;
        }else{
            std::cout << "IsBroadcastMoveMessage broadcast success" << std::endl;
        }
        */
        return squareDistance > squareMaxDis_;
     } 

protected:
    void ProcessMoveMessage(int entityId, EntityInfo& entityInfo, const std::vector<int>& oldNeighborsEntitys)  {
        if(!msgNotifyer_)
        {
            return;
        }

        // 判断是否需要同步, 需要则同步。
        auto newNeighborsEntitys = this->GetNeighbors(entityId);
        bool bBoradMoveMessage = this->IsBroadcastMoveMessage(entityInfo.x, entityInfo.y, entityInfo.broadcastX, entityInfo.broadcastY);
        if(bBoradMoveMessage)
        {
            // 如果有定时器则需要先关闭定时器
            if(entityInfo.timerId > 0)
            {
                int64_t timer_id = entityInfo.timerId;
                parititionedPool_->CancelTimer(threadIdx_, entityInfo.timerId);
                entityInfo.timerId = 0;
                // std::cout << "ProcessMoveMessage cancelTimer timerid:=" << timer_id << std::endl;
            }

            entityInfo.broadcastX = entityInfo.x;
            entityInfo.broadcastY = entityInfo.y;
           
            msgNotifyer_->BroadcastMsgForMoveAction(AOIMsgNotifyer::AOI_ALL, entityId, entityInfo.x, entityInfo.y, oldNeighborsEntitys, newNeighborsEntitys);
        }
        else
        {
            // 如果已经有定时器了，则等待定时器的到来，当前不做处理
            if(entityInfo.timerId > 0)
            {
                return;
            }

            // 
            // 只同步新增和删除
            // std::cout << "ProcessMoveMessage delayBroadcast entityId:=" << entityId << std::endl;
            //int broadcastMask = (AOIMsgNotifyer::AOI_ADD | AOIMsgNotifyer::AOI_DEL);
            //msgNotifyer_->BroadcastMsgForMoveAction(broadcastMask, entityId, entityInfo.x, entityInfo.y, oldNeighborsEntitys, newNeighborsEntitys);

            // 距离太近不需要同步的，设置一个定时器到时间了再同步
            auto delayMoveBroadcastFunc = [&entityInfo, entityId, &oldNeighborsEntitys, this](){
                entityInfo.timerId = 0;
                // 多次移动又重新移动会最开始的广播位置则不进行广播同步，避免频繁的广播
                if(entityInfo.x == entityInfo.broadcastX && entityInfo.y == entityInfo.broadcastY)
                {
                    // std::cout << "ProcessMoveMessage delay broadcast Function entityId:=" << entityId << " no need broadcast" << std::endl;
                    return;
                }
                
                entityInfo.broadcastX = entityInfo.x;
                entityInfo.broadcastY = entityInfo.y;

                 // std::cout << "ProcessMoveMessage delay broadcast Function entityId:=" << entityId << std::endl;

                if(msgNotifyer_)
                {
                    auto newNeighborsEntitys =  this->GetNeighbors(entityId);
                    msgNotifyer_->BroadcastMsgForMoveAction(AOIMsgNotifyer::AOI_ALL, entityId, entityInfo.x, entityInfo.y, oldNeighborsEntitys, newNeighborsEntitys);
                }
    
            };

            entityInfo.timerId = parititionedPool_->DelayRunOnce(threadIdx_, delaySecondBroadcastMove_, delayMoveBroadcastFunc);
        }

    }


protected:
    int gridSize_;
    std::unique_ptr<AOIMsgNotifyer> msgNotifyer_;

    int threadIdx_;
    int squareMaxDis_ = 0; // 超过这个距离的阈值则进行同步move消息
    std::shared_ptr<PartitionedPool> parititionedPool_;
    int delaySecondBroadcastMove_;

    //std::shared_ptr<DeltaSyncManager> delaSyncMgr_;
};