#pragma once

#include <string>
#include <unordered_map>
#include <functional>
#include <fstream>
#include <thread>
#include <memory>
#include <atomic>
#include <mutex>

class HttpServer;
class ServiceRegisterCenter
{
public:
    ServiceRegisterCenter(HttpServer* http_server);

private:
    void InitServicesRegisterCenter();

private:
    HttpServer* http_server_; // 保存裸指针

    // 一个服务注册实例
    struct ServerInstance
    {
        std::string ip;
        int port;
        std::chrono::steady_clock::time_point last_heartbeat;
        int ttl_secs;
    };

    std::unordered_map<std::string, std::vector<ServerInstance>> services_register_center_;
    std::mutex register_center_mutex_;
};