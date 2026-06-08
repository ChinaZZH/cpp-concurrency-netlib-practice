#pragma once

#include <memory>
#include <thread>
#include "TcpServer.h"
#include "HttpContext.h"

class EventLoop;

class HttpServer
{
public:
    using Handler = std::function<std::string(const std::string&)>;

    HttpServer(EventLoop* loop, int nPort);

    void Start(int option, int nEventLoopThread, int nTaskThreadNum = std::thread::hardware_concurrency());

    void RegisterMethod(const std::string& strMethod, Handler handler);

private:
    // 同步 
    void OnMessage(const std::shared_ptr<TcpConnection>& con, std::string& strMsg);

    void SendHttpResponse(const std::shared_ptr<TcpConnection>& con, const std::string& strContent, int nStatusCode = 200);

    // 通过线程池异步执行
    void AsyncOnMessage(const std::shared_ptr<TcpConnection>& con, std::string& strMsg);

    void AsyncSendHttpResponse(EventLoop* loop, const std::weak_ptr<TcpConnection>& conWeakPtr, const std::string& strContent, int nStatusCode);

    Handler GetHadlerByMethod(const std::string& strMethod);

    std::string GetStatusCodeMsg(int nStatusCode);

private:
    TcpServer server_;
    HttpContext http_server_context_;
    std::unordered_map<std::string, Handler> method_handlers_;
};