#pragma once 

#include <vector>
#include <memory>
#include <functional>
#include <string>

class EventLoop;
class EventLoopThread;
class TcpServer;

class EventLoopThreadPool
{
public:
    using ThreadInitallback = std::function<void(EventLoop*)>;

    EventLoopThreadPool(EventLoop* baseLoop, const std::string& nameArg="");

    ~EventLoopThreadPool();

    void SetThreadNum(int threadCount) { numThreads_ = threadCount; }
    void Start(TcpServer* tcpServer, int idleSecTimeOut, const ThreadInitallback& cb = ThreadInitallback());

    // 轮询分配EventLoop(负载均衡)
    EventLoop* getNextLoop();

    // 返回所有的EventLoop(包括baseLoop_)
    std::vector<EventLoop*> GetAllLoops() const;

    bool IsStarted() const { return started_; }

private:
    EventLoop* baseLoop_; // 主线程的eventLoop用于accept
    std::string name_;
    bool started_;
    int numThreads_;    // 工作线程数量
    int next_;          // 轮询索引
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    std::vector<EventLoop*> loops_;
};