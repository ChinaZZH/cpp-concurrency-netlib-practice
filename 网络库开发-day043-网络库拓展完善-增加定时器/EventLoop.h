// EventLoop.h

#pragma once

#include <vector>
#include <map>
#include <memory>
#include <thread>
#include <mutex>
#include <functional>

class Channel;
class Epoll;
class ClientSocket;
class TcpServer;

class EventLoop
{
public:
    EventLoop();

    ~EventLoop();

public:
    void InitServer(TcpServer* tcpServer) { tcpServer_ = tcpServer; }

    // 启动事件循环
    void Loop();

    // 停止事件循环
    void Quit();

    // 添加或更新channel(线程安全)
    void AddChannel(std::unique_ptr<Channel> channel, std::string strInfo = "OtherError");

    void DelayRemoveQueue(int fd);

    void AddEventToUpdateChannel(int fd, int event);

    void DelEventToUpdateChannel(int fd, int event);

public:
    // 跨线程调度: 如果当前是IO线程则直接执行，否则放入队列
    void RunInLoop(std::function<void()> cb);
    void QueueInLoop(std::function<void()> cb);
    void AssertInLoopThread(std::string strInfo);

    std::thread::id GetReactorThreadId() const { return threadId_; }
private:
     bool IsInLoopThread() const;

     // 移除Channel
     void RemoveChannel(int fd);

     void DoPendingFunctors();
     void WakeUp();              // 用于唤醒epoll_wait

// 新增定时器
public:
    // 单次定时器 delay为delay毫秒执行回调
    void RunAfter(std::chrono::milliseconds delay, std::function<void()> funcCb);

private:
    void HandleTimerRead();
    void UpdateTimerFd();
    void ExecuteExpiredTimers();

private:
    std::unique_ptr<Epoll>  epoll_;
    std::map<int, std::unique_ptr<Channel>> channels_;
    bool quit_;
    std::thread::id threadId_;
    std::vector<int> delayChannelsToRemove_;

    std::vector<std::function<void()>> pendingFunctors_;
	std::mutex mutex_;

    int wakeUpFd_;
    //std::unique_ptr<Channel> wakeUpChannel_; 融入到channels_列表中去了，统一管理

    TcpServer* tcpServer_; //保存裸指针
    
    int timerFd_;
    //std::unique_ptr<Channel> timerChannel_; 融入到channels_列表中去了，统一管理
    // 存储所有定时器：按超时时间排序（multimap 按时间自动排序）
    std::multimap<std::chrono::steady_clock::time_point, std::function<void()>> timersFunc_;
};