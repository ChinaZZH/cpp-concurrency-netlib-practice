#include "HttpServer.h"
#include "HttpContext.h"
#include "../TcpConnection.h"
#include "../EventLoop.h"
#include "../Decoder/HttpContentDecoder.h"
#include <sstream>
#include <cassert>
#include <functional>
#include <nlohmann/json.hpp>

HttpServer::HttpServer(EventLoop* loop, int nPort)
:server_(loop, nPort)
{
    server_.SetMessageCallBack(std::bind(&HttpServer::AsyncOnMessage, 
        this, std::placeholders::_1, 
        std::placeholders::_2)
    );

    server_.SetConnectionCallBack([](const std::shared_ptr<TcpConnection>& con){
        auto http_decoder = std::make_unique<HttpContentDecoder>();
        con->SetDecoder(std::move(http_decoder));
    });
}

void HttpServer::Start(int option, int nEventLoopThread, int nTaskThreadNum /*= std::thread::hardware_concurrency()*/)
{
    server_.Start(option, nEventLoopThread, nTaskThreadNum);
}



// 异步实现 注释版本同步实现 因为解析这个过程计算过程很快，所以这个仅仅是用来学习，实际过程中应该使用同步
void HttpServer::AsyncOnMessage(const std::shared_ptr<TcpConnection>& con, std::string& strMsg)
{
    ThreadPool* pool = server_.GetThreadPool();
    if(!pool)
    {
        return;
    }


    EventLoop* loop = con->GetLoop();
    loop->AssertInLoopThread("HttpServer::AsyncOnMessage");
    std::weak_ptr<TcpConnection> weakConPtr = con;

     // 将业务逻辑提交到线程池处理
    pool->AddTask([this](EventLoop* loop, const std::weak_ptr<TcpConnection>& conWeakPtr, std::string strMsg){
        //auto con = weakConPtr.lock();
        //if(con)
        {
            // 模拟耗时业务（例如解析、数据库查询）
            thread_local HttpContext async_thread_context;
            async_thread_context.Reset();
            bool bOk = async_thread_context.ParseRequest(strMsg);
            if(!bOk)
            {
                // 解析失败或需要更多数据，这里简单返回 400
                std::string body = "Bad Request";
                AsyncSendHttpResponse(loop, conWeakPtr, body, 400);
                return;
            }

            
            const std::string& strMethod = async_thread_context.GetMethod();
            if("POST" != strMethod)
            {
                // 简单路由：返回 "Hello, World!"
                std::string body = "<h1>Hello, World!</h1>";
                AsyncSendHttpResponse(loop, conWeakPtr, body, 200);
                return;
            }

            const std::string& strPath = async_thread_context.GetPath();
            std::string strHandleKey(strPath.data() + 1, strPath.size()-1);  
            auto func = this->GetHadlerByMethod(strHandleKey);
            if(!func)
            {
                // 简单路由：返回 "Hello, World!"
                std::string body = "Not Found";
                AsyncSendHttpResponse(loop, conWeakPtr, body, 404);
                return;
            }

            {
                const std::string& strBody = async_thread_context.GetBody();
                std::string result;
                try
                {
                    result = func(strBody);
                }
                catch(const std::exception& e)
                {
                    std::cerr << "HttpServer func error: " << e.what() << std::endl;
                }
                        
                AsyncSendHttpResponse(loop, conWeakPtr, result, 200);
            }
        }
        
    }, loop, weakConPtr, std::move(strMsg));
}

void HttpServer::AsyncSendHttpResponse(EventLoop* loop, const std::weak_ptr<TcpConnection>& conWeakPtr, const std::string& strContent, int nStatusCode)
{
    if(!loop)
    {
        return;
    }

    std::string strCode = std::move(GetStatusCodeMsg(nStatusCode));
    std::ostringstream response;
    response << "HTTP/1.1 " << nStatusCode << " " << strCode.c_str() << " \r\n"
             << "Content-Length: " << strContent.size() << "\r\n"
             << "Content-Type: text/html\r\n"
             << "\r\n"
             << strContent;


    std::string strResponse = response.str();
    //std::weak_ptr<TcpConnection> weakConPtr = con;
    
    {
        loop->RunInLoop([this, conWeakPtr, strResponse = std::move(strResponse)](){ 
            //if(weakConPtr.lock()) weakConPtr.lock()->Send(strResponse);
                std::shared_ptr<TcpConnection> con = conWeakPtr.lock(); 
                if(con && !con->IsWriteClosed())
                {
                    if(con->IsPause())
                    {
                        con->WaitForWaterToLowMask(strResponse);
                    }else{
                        con->Send(strResponse);
                    }
                } 
        });
    }
    
}


// 同步实现版本
void HttpServer::OnMessage(const std::shared_ptr<TcpConnection>& con, std::string& strMsg)
{
    http_server_context_.Reset();
    bool bOk = http_server_context_.ParseRequest(strMsg);
    if(!bOk)
    {
        // 解析失败或需要更多数据，这里简单返回 400
        std::string body = "Bad Request";
        SendHttpResponse(con, body, 400);
        return;
    }

    const std::string& strMethod = http_server_context_.GetMethod();
    if("POST" != strMethod)
    {
        // 简单路由：返回 "Hello, World!"
        std::string body = "<h1>Hello, World!</h1>";
        SendHttpResponse(con, body, 200);
    }

    const std::string& strPath = http_server_context_.GetPath();
    std::string strHandleKey(strPath.data() + 1, strPath.size()-1);  
    auto func = this->GetHadlerByMethod(strHandleKey);
    if(func)
    {
        const std::string& strBody = http_server_context_.GetBody();
        std::string result = func(strBody);
        SendHttpResponse(con, result, 200);
    }
    else
    {     
        // 简单路由：返回 "Hello, World!"
        std::string body = "Not Found";
        SendHttpResponse(con, body, 404);
    }
}

void HttpServer::SendHttpResponse(const std::shared_ptr<TcpConnection>& con, const std::string& strContent, int nStatusCode)
{
    std::string strCode = std::move(GetStatusCodeMsg(nStatusCode));
    std::ostringstream response;
    response << "HTTP/1.1 " << nStatusCode << " " << strCode.c_str() << " \r\n"
             << "Content-Length: " << strContent.size() << "\r\n"
             << "Content-Type: text/html\r\n"
             << "\r\n"
             << strContent;

    if(con->IsPause())
    {
        con->WaitForWaterToLowMask(response.str());
    }else{
        con->Send(response.str());
    }
}


void HttpServer::RegisterMethod(const std::string& strMethod, Handler handler)
{
    method_handlers_[strMethod] = handler;
}


HttpServer::Handler HttpServer::GetHadlerByMethod(const std::string& strMethod)
{
    auto itr = method_handlers_.find(strMethod);
    if(itr == method_handlers_.end())
    {
        return nullptr;
    }

    return (itr->second);
}


std::string HttpServer::GetStatusCodeMsg(int nStatusCode)
{
    std::string strCode;
    if(200 == nStatusCode)
    {
        strCode.assign("OK");
    }
    else if(400 == nStatusCode)
    {
        strCode.assign("Bad Request");
    }
    else if(404 == nStatusCode)
    {
        strCode.assign("Not Found");

    }
    else
    {
        strCode.assign("ERROR");
    }

    return strCode;
}


 void HttpServer::InitServicesRegisterCenter()
 {
    using json = nlohmann::json;
    is_services_center = true;
    
    EventLoop* base_loop = server_.GetMainLoop();
    assert(base_loop);

    // 注册检测定时器
    std::chrono::seconds checkSecs(10);
    base_loop->RunEvery(checkSecs, [this](){
        {
            auto now = std::chrono::steady_clock::now();
            std::lock_guard<std::mutex> lk(register_center_mutex_);
            for(auto& instance : services_register_center_)
            {
                auto& vecInstance = (instance.second);
                auto itr_delete = std::remove_if(vecInstance.begin(), vecInstance.end(), [now](const ServerInstance& instance){
                    auto keep_second = std::chrono::duration_cast<std::chrono::seconds>(now - instance.last_heartbeat).count();
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

    this->RegisterMethod("register", [this](const std::string& body_param) -> std::string{
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
   this->RegisterMethod("heartbeat", [this](const std::string& body_param) -> std::string{
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
    this->RegisterMethod("discover", [this](const std::string& body_param) -> std::string{
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

            return json({{"instances", instances}}).dump();
        }
        catch(...)
        {
           return R"({"code":-1, "msg":"invalid json"})";
        }
        
        return R"({"code":-1, "msg":"invalid json"})";
    });
 }