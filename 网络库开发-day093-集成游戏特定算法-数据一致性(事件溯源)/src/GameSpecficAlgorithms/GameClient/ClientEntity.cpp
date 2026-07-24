#include "ClientEntity.h"

ClientEntity::ClientEntity(int id, float x, float y)
:entityId_(id)
,currentX_(x)
,currentY_(y)
,duration_(0.2f) // 插值时长（200ms） 0.2秒, 根据视觉表现的实际情况进行调整。
,bMoving_(false)
,startTime_(std::chrono::steady_clock::now())
{

}


void ClientEntity::OnMoveAction(float newX, float newY)
{
    if(currentX_ == newX && currentY_ == newY)
    {
        return ;
    }

    std::cout << "ClientEntity::OnMoveAction entityId_:=" << entityId_ << " srcPostion:=(" << currentX_ << " ," << currentY_;
    std::cout << " ) targetPostion:=("  << newX << ", " << newY << ")" << std::endl;

    bMoving_ = true;
    moving_.srcX_ = currentX_;
    moving_.srcY_ = currentY_;
    moving_.targetX_ = newX;
    moving_.targetY_ = newY;
    moving_.remainProgress_ = 1.0f;
}


void ClientEntity::UpdateLoop()
{
    if(false == bMoving_)
    {
        return;
    }

    // 使用平滑插值  duration_ 用 duration_系数是指用duration_秒数完成整个从src原点到target目标点，并且整个过程匀速
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    auto elapseSec = std::chrono::duration<float>(now - startTime_).count();
    startTime_ = now;

    float progressRatio = std::min(elapseSec / duration_, 1.0f);
    
    if(progressRatio >= moving_.remainProgress_)
    {
        currentX_ = moving_.targetX_;
        currentY_ = moving_.targetY_;

        bMoving_ = false;
        moving_.srcX_ = 0.0f;
        moving_.srcY_ = 0.0f;
        moving_.targetX_ = 0.0f;
        moving_.targetY_ = 0.0f;
        moving_.remainProgress_ = 0.0f; 
    }
    else{
        moving_.remainProgress_ -= progressRatio;

        float progress = (1.0f - moving_.remainProgress_);
        currentX_ += (moving_.targetX_ - moving_.srcX_) * progress;
        currentY_ += (moving_.targetY_ - moving_.srcY_) * progress;
    }

    std::cout << "ClientEntity::UpdateLoop entityId_:=" << entityId_ << " newPostion:=(" << currentX_ << " ," << currentY_;
    std::cout << " ) remainProgress:="  << moving_.remainProgress_ << std::endl;
}