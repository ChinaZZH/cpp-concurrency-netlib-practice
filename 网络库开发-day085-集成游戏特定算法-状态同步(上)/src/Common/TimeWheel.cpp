#include "TimeWheel.h"
#include <iostream>

TimeWheel::TimeWheel()
:currentTimePoint_(std::chrono::steady_clock::now())
{
    std::int64_t current_time = std::chrono::duration_cast<std::chrono::seconds>(currentTimePoint_.time_since_epoch()).count();
    current_time = current_time % DAY_KEEP_SECONDS;

    secondIndex_ = current_time % 60;
    minuteIndex_ = (current_time / 60) % 60;
    hourIndex_ =  current_time / 3600;
}


uint64_t TimeWheel::AddNewTimer(int delaySeconds, int loopCount, std::function<void()> callBack)
{
     // 当前定时器只支持24小时内的定时器，不支持更大跨度的定时器。
    if((delaySeconds <= 0) || (delaySeconds > DAY_KEEP_SECONDS))
    {
        return 0;
    }

    RunNexTick();

    // 定时器id自增1
    {
        nextTimerId_ += 1;
    }

    
    auto newTimerNode = std::make_shared<TimerNode>();
    newTimerNode->timerId =  nextTimerId_;
    newTimerNode->callBack_ =  callBack;
    newTimerNode->remainCount = loopCount;
    newTimerNode->delaySeconds = delaySeconds;
    newTimerNode->isValid = true;
    if(AddToTimerNodeList(currentTimePoint_, newTimerNode))
    {
        activeTimers_[nextTimerId_] = newTimerNode;
        return nextTimerId_;
    }

    return 0;
}


bool TimeWheel::CancelTimer(int cacelTimerId)
{
    auto itr = activeTimers_.find(cacelTimerId);
    if(itr == activeTimers_.end())
    {
        return false;
    }

    std::shared_ptr<TimerNode> timerNode = (itr->second);
    timerNode->isValid = false;

    activeTimers_.erase(itr);
    return true;
}


void TimeWheel::RunNexTick()
{
    std::int64_t current_time = std::chrono::duration_cast<std::chrono::seconds>(currentTimePoint_.time_since_epoch()).count();
    
    // 秒数没发生变化, 说明 current_time 还没过去
    auto newTimePoint = std::chrono::steady_clock::now();
    std::int64_t newCurrentTime = std::chrono::duration_cast<std::chrono::seconds>(newTimePoint.time_since_epoch()).count();
    if(current_time >= newCurrentTime)
    {
        return;
    }

    // 时间往前走一格了
    int addSeconds = newCurrentTime - current_time;
    // std::cout << "TimeWheel::RunNexTick addSeconds:=" << addSeconds << std::endl;

    for(int i = 1; i <= addSeconds; i += 1)
    {  
        // 则往前走一个， 如果到了新的分针的时候，则将分针拿出来
        secondIndex_ = (secondIndex_ + 1) % 60;
        if(0 == secondIndex_)
        {
            minuteIndex_ = (minuteIndex_ + 1) % 60;
            if(0 == minuteIndex_)
            {
                // 秒和分指针都为0，则表明走完一个时钟，则。时针+1.
                hourIndex_ = (hourIndex_ + 1) % 24;
                this->NewHourStart();
            }
            
            this->NewMinuteStart();
        }

        // 秒数发生变化，则最多秒针往前走1格, 每200毫秒定时一次
        // 先执行完当前secondIndex_里面的定时器函数
        auto offsetTimePoint = currentTimePoint_ + std::chrono::seconds(i); 
        TimerNodeList& nodeList = secondTimeNode_[secondIndex_];
        while(!nodeList.empty())
        {
            std::shared_ptr<TimerNode> timerNode = nodeList.front();
            nodeList.pop();
            if(false == timerNode->isValid)
            {
                activeTimers_.erase(timerNode->timerId);
                continue;
            }

            (timerNode->callBack_)();
            if(1 == timerNode->remainCount)
            {
                // 执行完最后一次就结束该定时器任务，不继续做放入timerNode.
                activeTimers_.erase(timerNode->timerId);
            }else
            {
                // timerNode->remainCount == 0 为一直循环
                if(timerNode->remainCount > 1)
                {
                    timerNode->remainCount -= 1;
                }

                // 则继续往定时器里面塞
                this->AddToTimerNodeList(offsetTimePoint, timerNode);
            }
        }
    }
    

    currentTimePoint_ = newTimePoint;
}

void TimeWheel::NewHourStart()
{
    TimerNodeList& nodeList = hourTimeNode_[hourIndex_];
    while(!nodeList.empty())
    {
        std::shared_ptr<TimerNode> timerNode = nodeList.front();
        nodeList.pop();

        if(false == timerNode->isValid)
        {
            activeTimers_.erase(timerNode->timerId);
            continue;
        }

             
        minuteTimeNode_[timerNode->activeMinuteIndex_].push(timerNode);
    }
}

void TimeWheel::NewMinuteStart()
{
    TimerNodeList& nodeList = minuteTimeNode_[minuteIndex_];
    while(!nodeList.empty())
    {
        std::shared_ptr<TimerNode> timerNode = nodeList.front();
        nodeList.pop();

        if(false == timerNode->isValid)
        {
            activeTimers_.erase(timerNode->timerId);
            continue;
        }

             
        secondTimeNode_[timerNode->activeSecondIndex_].push(timerNode);
    }

}



bool TimeWheel::AddToTimerNodeList(std::chrono::steady_clock::time_point current, std::shared_ptr<TimerNode>& timerNode)
{
    // 系统运行可能超过一天
    // std::cout << "TimeWheel::AddToTimerNodeList second:=" << timerNode->delaySeconds << std::endl;
    auto delayTimePoint = current + std::chrono::seconds(timerNode->delaySeconds);
    std::int64_t delaySecs = std::chrono::duration_cast<std::chrono::seconds>(delayTimePoint.time_since_epoch()).count();

    int remainTotalSecs = delaySecs % DAY_KEEP_SECONDS; 
    int delaySecondIdx = remainTotalSecs % 60;
    int delayMinuteIdx = (remainTotalSecs / 60) % 60;
    int delayHourIdx =  remainTotalSecs / 3600;

    //timerNode->activeHourIndex_ = delayHourIdx;
    timerNode->activeMinuteIndex_ = delayMinuteIdx;
    timerNode->activeSecondIndex_ = delaySecondIdx;

     if(delayHourIdx != hourIndex_)
    {
        hourTimeNode_[delayHourIdx].push(timerNode);
    }
    else if(delayMinuteIdx != minuteIndex_)
    {
        minuteTimeNode_[delayMinuteIdx].push(timerNode);
    }
    else 
    {
        if(delaySecondIdx > secondIndex_)
        {
            secondTimeNode_[delaySecondIdx].push(timerNode);
        }
        else
        {
            std::cout << "TimeWheel::AddToTimerNodeList error timerId:=" << (timerNode->timerId) << " delaySeconds:=" << (timerNode->delaySeconds) << std::endl;
            return false;
        }
    }

    // std::cout << "TimeWheel::AddToTimerNodeList delayHourIdx:=" <<  delayHourIdx;
    // std::cout << " delayMinuteIdx:=" << delayMinuteIdx << " delaySecondIdx:=" << delaySecondIdx << std::endl;
    return true;
}