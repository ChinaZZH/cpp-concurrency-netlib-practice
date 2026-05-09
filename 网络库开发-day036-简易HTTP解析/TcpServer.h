#pragma once

#include <memory>
#include <functional>
#include <map>
#include <thread>


class EventLoop;
class ListenSocket;
class Channel;
class TcpConnection;
class ThreadPool;


class TcpServer
{
public:
    using MessageCallBack = std::function<void(const std::shared_ptr<TcpConnection>&, std::string&)>;
    using CloseCallBack = std::function<void(const std::shared_ptr<TcpConnection>&)>;

    TcpServer(EventLoop* loop, int nPort, int nThreadNum = std::thread::hardware_concurrency() - 1);
    
    ~TcpServer();

    void Start();

    void SetMessageCallBack(MessageCallBack cb) { messageCallBack_ = cb; }
    void SetCloseCallBack(CloseCallBack cb) { closeCallBack_ = cb; }

private:
    void HadleNewConnection();
    void RemoveConnection(const std::shared_ptr<TcpConnection>& conn);

private:
    EventLoop* loop_;
    int port_;
    std::unique_ptr<ListenSocket> listenSocket_;
    std::map<int, std::shared_ptr<TcpConnection>> mapTcpConnection_;

    MessageCallBack messageCallBack_;
    CloseCallBack closeCallBack_;
    std::unique_ptr<ThreadPool> threadPool_;
};