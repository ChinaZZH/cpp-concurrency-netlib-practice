#include "RpcConnectionPool.h"
#include "../Common/ConfigManager.h"
#include "RpcClient.h"
#include <stdexcept>

RpcConnectionPool::RpcConnectionPool()
{
    auto& cfg = ConfigManager::getInstance();

    std::string strIp = cfg.getString("RegisterCenter", "ip", "127.0.0.1");
    int port = cfg.getInt("RegisterCenter", "port", 8080);
    int pool_count = cfg.getInt("ConnectionPool", "count", 0);

    rpc_client_list_.reserve(pool_count);
    for(int i = 0; i < pool_count; ++i)
    {
        rpc_client_list_.emplace_back(std::make_shared<RpcClient>(strIp, port));
        rpc_client_list_[i]->AutoConnect();
    }

}
    
RpcConnectionPool::~RpcConnectionPool()
{

}

// rb轮询实现负载均衡
RpcConnectionPool::RpcClientPtr RpcConnectionPool::GetConnection()
{
    if(rpc_client_list_.empty())
    {
        throw std::runtime_error("RpcConnectionPool is empty");
    }


    uint64_t idx = next_id.fetch_add(1, std::memory_order_release);
    idx = idx % rpc_client_list_.size();
    auto client = rpc_client_list_[idx];
    if(client->IsConnected())
    {
        return client;
    }

    for(int i = 0; i < rpc_client_list_.size(); ++i)
    {
        uint64_t new_idx = (idx + i) % rpc_client_list_.size();
        if(rpc_client_list_[new_idx]->IsConnected())
        {
            return rpc_client_list_[new_idx];
        }
    }

    
    throw std::runtime_error("no avaiable connection in pool");
}


void RpcConnectionPool::CheckAndReconnect()
{

}