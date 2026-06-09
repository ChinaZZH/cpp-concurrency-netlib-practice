#pragma once 

#include <vector>
#include <atomic>
#include <memory>
#include <mutex>

class RpcClient;
class RpcConnectionPool
{
public:
    using RpcClientPtr = std::shared_ptr<RpcClient>;

    RpcConnectionPool();
    
    ~RpcConnectionPool();

    RpcClientPtr GetConnection();       // rb轮询实现负载均衡

    void CheckAndReconnect();           // 可选

private:
    std::vector<RpcClientPtr> rpc_client_list_;
    std::atomic<uint64_t>  next_id = 0;         // rb算法实现简单的负载均衡
    std::mutex reconnect_mutex_;
};