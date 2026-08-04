#pragma once
#include <queue>
#include <unordered_map>
#include <functional>
#include <string>
#include <fstream>
#include <thread>
#include <memory>
#include <chrono>

// 暂时设置以秒为精度的定时器 先实现以秒为单位的定时器，如果需要以毫秒后面再修改
struct TimerNode 
{
    uint64_t timerId;
    std::function<void()> callBack_;
    int remainCount;                    // 表示剩余次数 0表示永远不删除  1表示执行最后一次 N 表示执行最后N次
    int delaySeconds;                   // 延迟秒数
    bool isValid = true;                // 是否有效，默认有效，人为取消才无效。

    //int activeHourIndex_;
    int activeMinuteIndex_ = 0;
    int activeSecondIndex_ = 0;
};



class TimeWheel
{
public:
    TimeWheel();

    // 析构的不需要等待所有定时器任务执行完，执行完所有任务由上层的业务调用接口来保证。
    ~TimeWheel() 
    {
        activeTimers_.clear(); // 智能指针会自动释放
    }

    // 定时器只支持24小时内的定时器，不支持更大跨度的定时器，如果有更大跨度的情况则直接返回失败
    uint64_t AddNewTimer(int delaySeconds, int loopCount, std::function<void()> callBack);

    bool CancelTimer(int cacelTimerId);

    void RunNexTick();

private:
    // 跑完一小时的定时器内容，则从
    void NewHourStart();

    void NewMinuteStart();

    bool AddToTimerNodeList(std::chrono::steady_clock::time_point current, std::shared_ptr<TimerNode>& timerNode);

private:
    uint64_t nextTimerId_ = 0; // 下一次的定时器id
    
     using TimerNodeList = std::queue<std::shared_ptr<TimerNode>>;
    TimerNodeList hourTimeNode_[24];    // 时针表
    TimerNodeList minuteTimeNode_[60];  // 分针表
    TimerNodeList secondTimeNode_[60];  // 秒针表
    //TimerNodeList millSecTimeNode_[1000];  // 毫秒针表

    // 当前的时针，时针分针秒针
    int hourIndex_ = 0;
    int minuteIndex_ = 0;
    int secondIndex_ = 0;
    //int millSecIndex_ = 0;

    std::unordered_map<uint64_t, std::shared_ptr<TimerNode>> activeTimers_;
    std::chrono::steady_clock::time_point currentTimePoint_;                    // 当前时间,服务器tick时间不取系统时间

    const static int DAY_KEEP_SECONDS = 24*60*60;
};


 