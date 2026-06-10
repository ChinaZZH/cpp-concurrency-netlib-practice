#pragma once

#include "../TcpServer.h"
#include <unordered_map>
#include <functional>
#include <string>
#include <fstream>
#include <thread>
#include <memory>
#include <atomic>

class EventLoop;
class ServiceRegistry;
class RpcServer
{
public:
    using Handler = std::function<std::string(const std::string&)>;

    RpcServer(EventLoop* loop, int nPort);

    ~RpcServer();

    void Start(int option, int nEventLoopThread, int nTaskThreadNum = std::thread::hardware_concurrency());

    void RegisterMethod(const std::string& strMethod, Handler handler);

    void EnableServiceDiscovery(const std::string& registry_host, int registry_port, 
        const std::string& service_name, const std::string& my_ip, int my_port, int ttl_sec);

private:
    // 同步处理，不走任务线程池
    void OnMessage(const std::shared_ptr<TcpConnection>& con, std::string& strMsg);

    void HandlerResultResponse(const std::shared_ptr<TcpConnection>& con, uint64_t id, int32_t code, std::string strResult);

private:
    TcpServer server_;
    std::unordered_map<std::string, Handler> methods_;

    std::unique_ptr<ServiceRegistry> service_registry_;
};