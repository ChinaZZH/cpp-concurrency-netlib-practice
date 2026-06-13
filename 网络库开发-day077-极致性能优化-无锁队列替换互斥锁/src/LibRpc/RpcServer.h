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

    void Start();

    void RegisterMethod(const std::string& strMethod, Handler handler);

    void EnableServiceDiscovery(const std::string& registry_host, int registry_port, 
        const std::string& service_name, const std::string& my_ip, int my_port, int ttl_sec);

private:
    // 同步处理，不走任务线程池
    void OnMessage(const std::shared_ptr<TcpConnection>& con, std::string& strMsg);

    void HandlerResultResponse(EventLoop* loop_ptr, const std::weak_ptr<TcpConnection>& weak_con,  uint64_t id, int32_t code, const std::string& strResult, const std::string& trace_id);

private:
    TcpServer server_;
    std::unordered_map<std::string, Handler> methods_;

    std::unique_ptr<ServiceRegistry> service_registry_;
};