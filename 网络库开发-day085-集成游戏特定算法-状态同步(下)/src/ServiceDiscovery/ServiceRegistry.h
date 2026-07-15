#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <fstream>
#include <thread>
#include <memory>
#include <atomic>
#include <mutex>


class ServiceRegistry
{
public:
    ServiceRegistry();

    ~ServiceRegistry();
    
public:
    void EnableServiceDiscovery(const std::string& registry_host, int registry_port, 
        const std::string& service_name, const std::string& my_ip, int my_port, int ttl_sec);

private:
    // 发送心跳消息到服务注册中心，由于现在的httpClient实现的post和get是同步阻塞的，所以必须新开一个线程去做发送。
    std::atomic<bool> stop_heartbeat_ = false;
    std::unique_ptr<std::thread>  heartbeat_thread_;
};