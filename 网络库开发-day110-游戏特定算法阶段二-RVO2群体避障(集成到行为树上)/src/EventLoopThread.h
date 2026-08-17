#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <functional>
#include <memory>

class EventLoop;

class EventLoopThread
{
public:
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback(), const std::string& name = std::string());

    ~EventLoopThread();

    // 启动线程后，返回该线程中的EventLoop指针
    EventLoop* StartLoop();
    
private:
    void ThreadFunc();  // 线程入口函数

private:
    EventLoop* loop_;          // 子线程创建的 EventLoop对象
    bool exiting_;                  // 退出标志
    std::unique_ptr<std::thread> thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback callback_;   // 在eventLoop创建后，loop前执行
    std::string name_;
};