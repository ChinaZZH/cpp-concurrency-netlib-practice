#pragma once

#include <memory>
#include <thread>
#include "TcpServer.h"
#include "HttpContext.h"

class EventLoop;

class HttpServer
{
public:
    HttpServer(EventLoop* loop, int nPort);

    void Start(int option, int nEventLoopThread = std::thread::hardware_concurrency() - 1, int nTaskThreadNum = std::thread::hardware_concurrency() - 1);

private:
    // 同步 
    void OnMessage(const std::shared_ptr<TcpConnection>& con, std::string& strMsg);

    void SendHttpResponse(const std::shared_ptr<TcpConnection>& con, const std::string& strContent, int nStatusCode = 200);

    // 通过线程池异步执行
    void AsyncOnMessage(const std::shared_ptr<TcpConnection>& con, std::string& strMsg);

    void AsyncSendHttpResponse(EventLoop* loop, const std::weak_ptr<TcpConnection>& conWeakPtr, const std::string& strContent, int nStatusCode);

private:
    TcpServer server_;
    HttpContext context_;
};