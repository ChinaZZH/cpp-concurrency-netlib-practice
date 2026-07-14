#pragma once

#include "IAOIManager.h"
#include "AOIMsgNotifyer.h"
#include <unordered_map>
#include <mutex>
#include <memory>
#include <cstdlib> 


// ============================================================
// 实体信息
// ============================================================
struct EntityInfo
{
    int x;
    int y;

    int gridX;
    int gridY;
};

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

     virtual bool IsInRangeForGridPosition(int gridX1, int gridY1, int gridX2, int gridY2, int radius) const override {
        return std::abs(gridX1 - gridX2) <= radius && std::abs(gridY1 - gridY2) <= radius;
     }

     virtual bool IsInRange(int x1, int y1, int x2, int y2, int radius) const override {
        std::pair<int, int> prePos = this->GetGridCoord(x1, y1);
        std::pair<int, int> nextPos = this->GetGridCoord(x2, y2);
        return this->IsInRangeForGridPosition(prePos.first, prePos.second, nextPos.first, nextPos.second, radius);
     }

protected:
    int gridSize_;
    std::unique_ptr<AOIMsgNotifyer> msgNotifyer_;
};