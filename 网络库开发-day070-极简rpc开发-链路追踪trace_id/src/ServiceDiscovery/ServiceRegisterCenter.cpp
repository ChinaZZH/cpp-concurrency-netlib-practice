#include "ServiceRegisterCenter.h"
#include "../EventLoop.h"
#include "../Http/HttpServer.h"
#include <sstream>
#include <cassert>
#include <functional>
#include <nlohmann/json.hpp>

ServiceRegisterCenter::ServiceRegisterCenter(HttpServer* http_server)
:http_server_(http_server)
{
    InitServicesRegisterCenter();
}

void ServiceRegisterCenter::InitServicesRegisterCenter()
{
    assert(http_server_);
    using json = nlohmann::json;
    
    EventLoop* base_loop = http_server_->server_.GetMainLoop();
    assert(base_loop);

    // 注册检测定时器
    std::chrono::seconds checkSecs(10);
    base_loop->RunEvery(checkSecs, [this](){
        {
            auto now = std::chrono::steady_clock::now();
            std::lock_guard<std::mutex> lk(register_center_mutex_);
            for(auto& instance : services_register_center_)
            {
                const std::string& service_name = instance.first;
                auto& vecInstance = (instance.second);
                auto itr_delete = std::remove_if(vecInstance.begin(), vecInstance.end(), [&service_name, now](const ServerInstance& instance){
                    auto keep_second = std::chrono::duration_cast<std::chrono::seconds>(now - instance.last_heartbeat).count();
                    if(keep_second > instance.ttl_secs)
                    {
                        //测试使用
                        std::cout << "auto remove instance service_name:" << service_name << " ip:" << instance.ip << " port:" << instance.port << std::endl;
                    }
                    return keep_second > instance.ttl_secs;
                });

                vecInstance.erase(itr_delete, vecInstance.end());
            }
        }
    });

    // /register
    /*
    request
    {
        "service": "rpc_server",   // 服务名称，字符串
        "ip": "192.168.1.100",      // 服务端IP
        "port": 8888,               // 服务端口
        "ttl": 30                   // 心跳超时时间（秒），可选，默认30
    }

    repsonse
    {"code":0}   // 成功
    {"code":-1, "msg":"error"}   // 失败
    */

    http_server_->RegisterMethod("register", [this](const std::string& body_param) -> std::string{
        try
        {
            auto json_body = json::parse(body_param);
            std::string strService = std::move(json_body["service"]);

            ServerInstance instance;
            instance.ip = std::move(json_body["ip"]);
            instance.port = json_body["port"];
            instance.ttl_secs = json_body["ttl"];
            instance.last_heartbeat = std::chrono::steady_clock::now();

            {
                std::lock_guard<std::mutex> lk(register_center_mutex_);
                auto& vecInstance = services_register_center_[strService];
                auto itr = std::find_if(vecInstance.begin(), vecInstance.end(), [&instance](const ServerInstance& old){
                    return (old.ip == instance.ip && old.port == instance.port);
                });

                if(itr == vecInstance.end())
                {
                    vecInstance.push_back(instance);
                }else{
                    ServerInstance& updateInstance = (*itr);
                    updateInstance.ttl_secs = instance.ttl_secs;
                    updateInstance.last_heartbeat = std::chrono::steady_clock::now();
                }
            }
            
            return R"({"code":0})";
        }
        catch(...)
        {
            return R"({"code":-1, "msg":"invalid json"})";
        }

        return R"({"code":-1, "msg":"invalid json"})";
        
    });

    // /heartbeat
    /*
    request
    {
        "service": "rpc_server",
        "ip": "192.168.1.100",
        "port": 8888
    }

    repsonse
    {"code":0}   // 成功
    {"code":-1, "msg":"error"}   // 失败
    */
   http_server_->RegisterMethod("heartbeat", [this](const std::string& body_param) -> std::string{
        try
        {
            auto json_body = json::parse(body_param);
            std::string strService;
            std::string strIp;
            int nPort;
            try
            {
                strService = std::move(json_body["service"]);
                strIp = std::move(json_body["ip"]);
                nPort = json_body["port"];
            }
            catch(const std::exception& e)
            {
                std::cerr << e.what() << '\n';
                return R"({"code":-1, "msg":"invalid json"})";
            }
            

            {
                std::lock_guard<std::mutex> lk(register_center_mutex_);
                auto itr = services_register_center_.find(strService);
                if(itr == services_register_center_.end())
                {
                    return R"({"code":-2, "msg":"instance not found"})";
                }

                auto& vecInstance = (itr->second);
                auto itr_instance = std::find_if(vecInstance.begin(), vecInstance.end(), [&strIp, nPort](const ServerInstance& old){
                    return (old.ip == strIp && old.port == nPort);
                });

                if(itr_instance == vecInstance.end())
                {
                    return R"({"code":-2, "msg":"instance not found"})";
                }

                auto& server_instance = (*itr_instance);
                server_instance.last_heartbeat = std::chrono::steady_clock::now();
            } 

            return R"({"code":0})";   // 成功

        }
        catch(...)
        {
            return R"({"code":-1, "msg":"invalid json"})";
        }

        return R"({"code":-1, "msg":"invalid json"})";
   });

    // /discover
    /*
    request
    {
        "service": "rpc_server"
    }

    response
    {
        "instances": [
            {"ip": "192.168.1.100", "port": 8888},
            {"ip": "192.168.1.101", "port": 8888}
        ]
    }
    */
    http_server_->RegisterMethod("discover", [this](const std::string& body_param) -> std::string{
        try
        {
            auto json_body = json::parse(body_param);
            std::string strService = std::move(json_body["service"]);
            json instances = json::array();

            do
            {
                std::lock_guard<std::mutex> lk(register_center_mutex_);
                auto itr = services_register_center_.find(strService);
                if(itr == services_register_center_.end())
                {
                    break;
                }

                const auto& vecInstance = (itr->second);
                for(const auto& inst : vecInstance)
                {
                    instances.push_back({{"ip", inst.ip}, {"port", inst.port}});
                }

                break;
            }while(0);

            return json({{"code", 0}, {"instances", instances}}).dump();
        }
        catch(...)
        {
           return R"({"code":-1, "msg":"invalid json"})";
        }
        
        return R"({"code":-1, "msg":"invalid json"})";
    });
}