#pragma once 

#include <vector>
#include <atomic>
#include <memory>
#include <mutex>
#include "../ServiceDiscovery/ServiceDiscovery.h"

class RpcClient;
class ServiceDiscovery;

class RpcConnectionPool
{
public:
    using RpcClientPtr = std::shared_ptr<RpcClient>;

    RpcConnectionPool();
    
    ~RpcConnectionPool();

    RpcClientPtr GetConnection();       // rb轮询实现负载均衡

public:
    // 手动刷新连接池(根据地址列表)
    void Refresh(const std::vector<EndPoint>& newPoints);

    // 启动自动发现(内部定时器刷新)
    void EnableAutoDiscovery(const std::string& registry_host, int registry_port
        , const std::string& service_name, int refresh_interval_sec = 30);

private:
    std::vector<RpcClientPtr> rpc_client_list_;
    std::atomic<uint64_t>  next_id = 0;         // rb算法实现简单的负载均衡
    std::mutex reconnect_mutex_;

    std::unique_ptr<ServiceDiscovery> service_discovery_;
    std::mutex service_refresh_mutex_;
};