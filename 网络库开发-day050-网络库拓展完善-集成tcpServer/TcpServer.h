#pragma once

#include <memory>
#include <functional>
#include <map>
#include <unordered_map>
#include <thread>
#include "ThreadPool.h"
#include "EventLoopThreadPool.h"


class EventLoop;
class ListenSocket;
class Channel;
class TcpConnection;


class TcpServer
{
public:
    using MessageCallBack = std::function<void(const std::shared_ptr<TcpConnection>&, std::string&)>;
    using CloseCallBack = std::function<void(const std::shared_ptr<TcpConnection>&)>;

    TcpServer(EventLoop* loop, int nPort);
    
    ~TcpServer();

    void Start(int option, int nEventLoopThread = std::thread::hardware_concurrency() - 1, int nTaskThreadNum = std::thread::hardware_concurrency() - 1);

    void SetMessageCallBack(MessageCallBack cb) { messageCallBack_ = cb; }
    void SetCloseCallBack(CloseCallBack cb) { closeCallBack_ = cb; }

    ThreadPool* GetThreadPool() { return taskThreadPool_.get(); }
    EventLoop* GetMainLoop() { return mainLoop_; }

    void HandleOnMessage(const std::shared_ptr<TcpConnection>& con, std::string& strMsg);
    void RemoveConnectionByFd(int fd);

    // 设置空闲时间
    void SetConnectionIdleTimeOut(int nSecs);
private:
    void HandleNewConnection();
    void ClosedConnection(const std::shared_ptr<TcpConnection>& conn);

    void CheckIdleConnections();

private:
    EventLoop* mainLoop_;
    int port_;
    std::unique_ptr<ListenSocket> listenSocket_;
    std::map<int, std::shared_ptr<TcpConnection>> mapTcpConnection_;

    MessageCallBack messageCallBack_;
    CloseCallBack closeCallBack_;
    std::unique_ptr<ThreadPool> taskThreadPool_;

    int idleTimeOutSecs_ = 60; // 连接空闲时间断开，默认是60秒(<=0 则空闲时间可以无限)

    std::unique_ptr<EventLoopThreadPool> eventLoopThreadPool_;
    int nEventLoopThreadCount_;
};