#pragma once

#include <iostream>
#include <chrono>
#include <memory>

/*
插值时长	适用场景
100 ms	低延迟环境，移动响应非常灵敏，适合竞技类游戏
200 ms	适合大部分游戏，平衡了平滑度与响应速度
300 ms	高延迟或网络波动较大的环境，移动更平滑，但响应稍慢
建议从 200 ms 开始，感觉“飘”就缩短，感觉“卡”就拉长。
*/

class ClientEntity
{
public:
    explicit ClientEntity(int id, float x, float y);
    ~ClientEntity() = default;

public:
    void OnMoveAction(float newX, float newY);
    
    void UpdateLoop();

    float GetCurrentX() const { return currentX_; }

    float GetCurrentY() const { return currentY_; }
    
private:
    int entityId_;
    float currentX_;
    float currentY_;
    float duration_ = 0.2f; // 插值时长（200ms） 0.2秒
    bool bMoving_ = false;
    std::chrono::steady_clock::time_point startTime_;

    struct MovingInfo
    {
        float srcX_;
        float srcY_;

        float targetX_;
        float targetY_;

        float remainProgress_;
    };

    MovingInfo moving_;
};