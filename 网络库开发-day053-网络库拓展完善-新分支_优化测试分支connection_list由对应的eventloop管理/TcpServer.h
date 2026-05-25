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


    ThreadPool* GetThreadPool() { return taskThreadPool_.get(); }
    EventLoop* GetMainLoop() { return mainLoop_; }

    // 设置空闲时间(暂时不用，看后面怎么向loop传递)
    void SetConnectionIdleTimeOut(int nSecs);

    void SetMessageCallBack(MessageCallBack cb) { messageCallBack_ = cb; }
    
    void SetCloseCallBack(CloseCallBack cb) { closeCallBack_ = cb; }

    void HandleOnMessage(const std::shared_ptr<TcpConnection>& con, std::string& strMsg);

    void RemoveClinetFd(int fd);

private:
    void HandleNewConnection();

private:
    EventLoop* mainLoop_;
    int port_;
    std::unique_ptr<ListenSocket> listenSocket_;
        
    std::unique_ptr<ThreadPool> taskThreadPool_;

    std::unique_ptr<EventLoopThreadPool> eventLoopThreadPool_;
    int nEventLoopThreadCount_;

    int idleTimeOutSecs_ = 60; // 连接空闲时间断开，默认是60秒(<=0 则空闲时间可以无限)

    MessageCallBack messageCallBack_;

    CloseCallBack closeCallBack_;

    std::map<int, EventLoop*> mapLightClient;
};