#pragma once

#include "IAOIManager.h"
#include "AOIMsgNotifyer.h"
#include <unordered_map>
#include <mutex>
#include <memory>


class BaseAOIManager : public IAOIManager {
public:
    explicit BaseAOIManager(int gridSize)
        : gridSize_(gridSize) {}

    virtual ~BaseAOIManager() = default;

    // 公共方法：广播回调注入（子类可直接复用）
    void SetSendMessageCallBack(SendMsgCallBack cb) override {
        //sendMsgCallBack_ = cb;
        msgNotifyer_ = std::make_unique<AOIMsgNotifyer>(cb, this);
    }

    virtual std::pair<int, int> GetGridCoord(int x, int y) const override {
        return std::pair(x / gridSize_, y / gridSize_);
    }

protected:
    int gridSize_;
    std::unique_ptr<AOIMsgNotifyer> msgNotifyer_;
};