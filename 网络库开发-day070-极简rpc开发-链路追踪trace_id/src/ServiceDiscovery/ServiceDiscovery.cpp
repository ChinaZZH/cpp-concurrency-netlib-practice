#include "ServiceDiscovery.h"
#include <nlohmann/json.hpp>
#include "../Http/SimpleHttpClient.h"
#include <iostream>
#include <vector>

 ServiceDiscovery::ServiceDiscovery(const std::string& registry_host, int registry_port, const std::string& service_name)
 :registry_host_(registry_host)
 ,registry_port_(registry_port)
 ,service_name_(service_name)
 {

 }


ServiceDiscovery::~ServiceDiscovery()
{
    stop_ = true;
    if(thread_ && thread_->joinable())
    {
        thread_->join();
    }
}

    
void ServiceDiscovery::Start(ServiceDiscovery::DiscoveryCallback cb, int refresh_interval_sec /*= 30*/)
{
    callback_ = cb;
    refresh_interval_sec_ = refresh_interval_sec;
    thread_ = std::make_unique<std::thread>(&ServiceDiscovery::RefreshLoop, this);
}


void ServiceDiscovery::RefreshLoop()
{
    using json = nlohmann::json;
    json json_discover_req = {{"service", service_name_}};
    std::string str_discover_req = json_discover_req.dump();

    while(!stop_.load())
    {
        auto res = SimpleHttpClient::Post(registry_host_, registry_port_, "/discover", str_discover_req, 3, 1);
        if(res.success && !res.body.empty())
        {
            std::cout << "ServiceDiscovery::RefreshLoop post success !!!" << std::endl;
            std::cout << "ServiceDiscovery::RefreshLoop body:=" << res.body << std::endl;
            try
            {
                auto json_res = json::parse(res.body);
                
                int code = json_res["code"];
                if(0 != code)
                {
                    // 发送的json串错误
                    std::cerr << "ServiceDiscovery::RefreshLoop discover json error" << std::endl;
                    return;
                }
                

                auto& instances = json_res["instances"];
                std::vector<EndPoint> endpoints;
                endpoints.reserve(instances.size());
                for(auto& inst  : instances)
                {
                    endpoints.push_back({inst["ip"], inst["port"]});
                    std::cout << "ServiceDiscovery::RefreshLoop end_point ip:=" << inst["ip"] << " port:=" << inst["port"] << " success !!!" << std::endl;
                }

                if(callback_)
                {
                    callback_(endpoints);
                    std::cout << "callback_ success !!!" << std::endl;
                }
            }
            catch(const std::exception& e)
            {
                throw std::runtime_error("ServiceDiscovery::RefreshLoop parse error");
            }
        }

        // 等待下次刷新
        for(int i = 0; i < refresh_interval_sec_; ++i)
        {
            if(stop_.load())
            {
                break;
            }

            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
    }
}