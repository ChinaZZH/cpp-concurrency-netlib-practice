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

    void Start(int option, int nEventLoopThread, int idleSecTimeOut, int nTaskThreadNum = std::thread::hardware_concurrency());


    ThreadPool* GetThreadPool() { return taskThreadPool_.get(); }
    EventLoop* GetMainLoop() { return mainLoop_; }

    void SetMessageCallBack(MessageCallBack cb) { messageCallBack_ = cb; }
    
    void SetCloseCallBack(CloseCallBack cb) { closeCallBack_ = cb; }

    void HandleOnMessage(const std::shared_ptr<TcpConnection>& con, std::string& strMsg);

    void RemoveClinetFd(int fd);

    // 暂时测试使用
     EventLoop* GetIndexLoop(int index) { return eventLoopThreadPool_->GetIndexLoop(index); }
private:
    void HandleNewConnection();

private:
    EventLoop* mainLoop_;
    int port_;
    std::unique_ptr<ListenSocket> listenSocket_;
        
    std::unique_ptr<ThreadPool> taskThreadPool_;

    std::unique_ptr<EventLoopThreadPool> eventLoopThreadPool_;
    int nEventLoopThreadCount_;

    MessageCallBack messageCallBack_;

    CloseCallBack closeCallBack_;

    std::map<int, EventLoop*> mapLightClient; // 轻量化客户端连接
};