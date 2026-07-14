#pragma once

#include "IAOIManager.h"
#include <unordered_map>
#include <mutex>

class IAOIManager;
class AOIMsgNotifyer {
public:
    using SendMsgCallBack = std::function<void(int entityId, const std::string& msg, GameServerMsgType )>;
    explicit AOIMsgNotifyer(SendMsgCallBack cb, IAOIManager* aoi)
    :sendMsgCallBack_(cb)
    ,aoiInterface_(aoi)
    { }

    ~AOIMsgNotifyer() = default;

public:
    // 公共广播方法（子类调用）
    void BroadcastToEntities(const std::vector<int>& entityList, const std::string& msg, GameServerMsgType msgType);

    void EnterNewGridPosMsgNotify(int entityId, int entityX, int entityY, const std::vector<int>& newNeighborsEntitys);

    void LeaveGridToMsgNotify(int entityId, const std::vector<int>& neighborsEntitys);

    void MovePostionToMsgNotify(int entityId, int entityX, int entityY, const std::vector<int>& neighborsEntitys);

    void MovePositionToMsgNotifyForGridChange(int entityId, int entityX, int entityY, const std::vector<int>& oldNeighborsEntitys, const std::vector<int>& newNeighborsEntitys);

protected:
    SendMsgCallBack sendMsgCallBack_;
    IAOIManager* aoiInterface_;
};