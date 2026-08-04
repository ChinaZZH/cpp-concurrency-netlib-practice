#include "ServiceRegistry.h"
#include "../Http/SimpleHttpClient.h"
#include <memory>
#include <nlohmann/json.hpp>
#include <iostream>

ServiceRegistry::ServiceRegistry()
{

}

ServiceRegistry::~ServiceRegistry()
{
    stop_heartbeat_ = true;
    if(heartbeat_thread_ && heartbeat_thread_->joinable())
    {
        heartbeat_thread_->join();
    }
}
    

void ServiceRegistry::EnableServiceDiscovery(const std::string& registry_host, int registry_port, 
    const std::string& service_name, const std::string& my_ip, int my_port, int ttl_sec)
{
    // 先发注册消息
    heartbeat_thread_ = std::make_unique<std::thread>([this, registry_host = std::move(registry_host), 
        registry_port, service_name = std::move(service_name), my_ip = std::move(my_ip), 
        my_port, ttl_sec] () {

        using json = nlohmann::json;
        json json_registry_req = {{"service", service_name}, {"ip", my_ip}, {"port", my_port}, {"ttl", ttl_sec}};
        std::string str_registry_req = json_registry_req.dump();

        json json_heartbeat_req = {{"service", service_name}, {"ip", my_ip}, {"port", my_port}};
        std::string str_heartbeat_req = json_heartbeat_req.dump();

        // 注册3次(重试3次)
        bool registry_result = false;
        for(int i = 0; i < 3; ++i)
        {
            SimpleHttpClient::Response response = SimpleHttpClient::Post(registry_host, registry_port, "/register", str_registry_req);
            if(response.success)
            {
                std::cout << "Service registered successfully" << std::endl;
                registry_result = true;
                break;
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }

        // 心跳循环
        int half_ttl_sec = ttl_sec / 2;
        std::chrono::seconds heartbeat_half_ttl(half_ttl_sec);
        while(!stop_heartbeat_.load())
        {
            std::this_thread::sleep_for(heartbeat_half_ttl);
            if(stop_heartbeat_.load())
            {
                break;
            }

            SimpleHttpClient::Response res = SimpleHttpClient::Post(registry_host, registry_port, "/heartbeat", str_heartbeat_req, 3, 1);
            if(!res.success)
            {
                std::cerr << "Heartbeat failed code:=" << res.status_code << " error_msg:=" << res.error_msg << std::endl;
            }else{
                //测试使用
                std::cout << "heartbeat succss!!!" << std::endl;
            }
        }
    });

    // 在定时的发心跳
}