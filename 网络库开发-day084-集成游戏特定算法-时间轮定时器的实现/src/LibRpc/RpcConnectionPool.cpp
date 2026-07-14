#include "RpcConnectionPool.h"
#include "../Common/ConfigManager.h"
#include "RpcClient.h"
#include <stdexcept>
#include <unordered_set>
#include <absl/container/flat_hash_set.h>

RpcConnectionPool::RpcConnectionPool() = default;
    
RpcConnectionPool::~RpcConnectionPool() = default;


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


// 手动刷新连接池(根据地址列表)
void RpcConnectionPool::Refresh(const std::vector<EndPoint>& newPoints)
{
    // 删除没有的ip port
    absl::flat_hash_set<EndPoint> new_instance_list(newPoints.begin(), newPoints.end());
    for(auto itr = rpc_client_list_.begin(); itr != rpc_client_list_.end(); )
    {
        RpcClientPtr client_ptr = (*itr);
        EndPoint old_point{client_ptr->GetIp(), client_ptr->GetPort()};
        if(!client_ptr->IsConnected() || 0 == new_instance_list.count(old_point))
        {
            itr = rpc_client_list_.erase(itr);
        }
        else{
            ++itr;
        }
    }

    // 加入新的ip port
     absl::flat_hash_set<EndPoint> old_instance_list;
     old_instance_list.reserve(rpc_client_list_.size());
     for(auto itr = rpc_client_list_.begin(); itr != rpc_client_list_.end(); )
     {
        RpcClientPtr client_ptr = (*itr);
        EndPoint old_point{client_ptr->GetIp(), client_ptr->GetPort()};
        old_instance_list.insert(old_point);
     }

    for(auto itr = newPoints.begin(); itr != newPoints.end(); ++itr)
    {
        const auto& new_point = (*itr);
        auto itr_add = old_instance_list.find(new_point);
        if(itr_add == old_instance_list.end())
        {
            auto new_rpc_client_ptr = std::make_shared<RpcClient>(new_point.ip, new_point.port);
            new_rpc_client_ptr->AutoConnect();
            rpc_client_list_.push_back(new_rpc_client_ptr);
        }
    }
}

// 启动自动发现(内部定时器刷新)
void RpcConnectionPool::EnableAutoDiscovery(const std::string& registry_host, int registry_port
        , const std::string& service_name, int refresh_interval_sec /*= 30*/)
{
 
    service_discovery_ = std::make_unique<ServiceDiscovery>(registry_host, registry_port, service_name);
    service_discovery_->Start(std::bind(&RpcConnectionPool::Refresh, this, std::placeholders::_1), refresh_interval_sec);
}
