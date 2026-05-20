#pragma once

#include <memory>
#include <functional>
#include <map>
#include <unordered_map>
#include <thread>
#include "ThreadPool.h"


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

    void Start(int option, int nThreadNum = std::thread::hardware_concurrency() - 1);

    void SetMessageCallBack(MessageCallBack cb) { messageCallBack_ = cb; }
    void SetCloseCallBack(CloseCallBack cb) { closeCallBack_ = cb; }

    ThreadPool* GetThreadPool() { return threadPool_.get(); }
    EventLoop* GetEventLoop()   { return loop_; }
    std::shared_ptr<TcpConnection> GetTcpConnection(int fd);

    void HandleOnMessage(const std::shared_ptr<TcpConnection>& con, std::string& strMsg);
    void RemoveConnectionByFd(int fd);

private:
    void HandleNewConnection();
    void ClosedConnection(const std::shared_ptr<TcpConnection>& conn);

private:
    EventLoop* loop_;
    int port_;
    std::unique_ptr<ListenSocket> listenSocket_;
    std::unordered_map<int, std::shared_ptr<TcpConnection>> mapTcpConnection_;

    MessageCallBack messageCallBack_;
    CloseCallBack closeCallBack_;
    std::unique_ptr<ThreadPool> threadPool_;
};