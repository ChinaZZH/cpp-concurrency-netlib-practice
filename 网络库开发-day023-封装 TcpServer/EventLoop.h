// EventLoop.h

#pragma once

#include <vector>
#include <map>
#include <memory>
#include <thread>

class Channel;
class Epoll;
class ClientSocket;

class EventLoop
{
public:
    EventLoop();

    ~EventLoop();

public:
    // 启动事件循环
    void Loop();

    // 停止事件循环
    void Quit();

    // 添加或更新channel(线程安全)
    void UpdateChannel(Channel *channel);


    // 移除Channel
     void RemoveChannel(Channel* channel);


private:
     void AssertInLoopThread();

     bool IsInLoopThread() const;

private:
    std::unique_ptr<Epoll>  epoll_;
    std::map<int, Channel*> channels_;
    bool quit_;
    std::thread::id threadId_;
};