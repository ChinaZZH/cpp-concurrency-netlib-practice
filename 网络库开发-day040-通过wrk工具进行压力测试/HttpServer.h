#pragma once

#include <memory>
#include <thread>
#include "TcpServer.h"


class HttpServer
{
public:
    HttpServer(EventLoop* loop, int nPort, int nThreadNum = std::thread::hardware_concurrency() - 1);

    void Start();

private:
    // 同步 
    void OnMessage(const std::shared_ptr<TcpConnection>& con, std::string& strMsg);

    void SendHttpResponse(const std::shared_ptr<TcpConnection>& con, const std::string& strContent, int nStatusCode = 200);

    // 通过线程池异步执行
    void AsyncOnMessage(const std::shared_ptr<TcpConnection>& con, std::string& strMsg);

    void AsyncSendHttpResponse(int fd, const std::string& strContent, int nStatusCode);

private:
    TcpServer server_;
};