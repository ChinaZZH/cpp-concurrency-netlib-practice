#pragma once

#include <memory>
#include <thread>
#include "HttpContext.h"
#include "../TcpServer.h"


class EventLoop;
class HttpWebService;
class HttpRouteParamHandler;

class HttpServer
{
public:
    using Handler = std::function<std::string(const std::string&)>;

    HttpServer(EventLoop* loop, int nPort);

    ~HttpServer();

    void Start();

    void RegisterMethod(const std::string& strMethod, Handler handler);

    // 防止安全调用加一个保护
    class SyncResponseToken
    {
    private:
        SyncResponseToken() = default;
        friend class HttpServer;
        friend class HttpRouteParamHandler;
        friend class HttpWebService;
    };

    static void SyncResponseToClient(
        const std::shared_ptr<TcpConnection>& con, 
        const std::string& strContent,
        bool bChunk, 
        const std::string& strTrunkData, 
        bool bKeepAlive, 
        SyncResponseToken token);

private:
    // 同步 
    void OnMessage(const std::shared_ptr<TcpConnection>& con, std::string& strMsg);

    void SendHttpResponse(const std::shared_ptr<TcpConnection>& con, const std::string& strContent, int nStatusCode, bool bKeepAlive);

    // 通过线程池异步执行
    void AsyncOnMessage(const std::shared_ptr<TcpConnection>& con, std::string& strMsg);

    void AsyncSendHttpResponse(EventLoop* loop, const std::weak_ptr<TcpConnection>& conWeakPtr, const std::string& strContent, int nStatusCode, bool bKeepAlive);

    Handler GetHadlerByMethod(const std::string& strMethod);

    static void SendChunkedData(const std::shared_ptr<TcpConnection>& con, const std::string& strContent);

private:
    TcpServer server_;
    HttpContext http_server_context_;
    std::unordered_map<std::string, Handler> method_handlers_;
    std::unique_ptr<HttpWebService> web_service_;
    std::unique_ptr<HttpRouteParamHandler> route_handler_;

    friend class ServiceRegisterCenter;
};