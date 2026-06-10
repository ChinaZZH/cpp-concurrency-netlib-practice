
#include "RpcServer.h"
#include "RpcCodec.h"
#include "RpcErrorCodeDef.h"
#include "../TcpConnection.h"
#include "../EventLoop.h"
#include "../Decoder/LengthPrefixDecoder.h"
#include "../Http/SimpleHttpClient.h"
#include <memory>
#include <nlohmann/json.hpp>

RpcServer::RpcServer(EventLoop* loop, int nPort)
:server_(loop, nPort)
{
    server_.SetMessageCallBack(std::bind(&RpcServer::OnMessage, 
        this, std::placeholders::_1, 
        std::placeholders::_2)
    );

    server_.SetConnectionCallBack([](const std::shared_ptr<TcpConnection>& con){
        auto length_decoder = std::make_unique<LengthPrefixDecoder>();
        con->SetDecoder(std::move(length_decoder));
    });
}


RpcServer::~RpcServer()
{
    stop_heartbeat_ = true;
    if(heartbeat_thread_ && heartbeat_thread_->joinable())
    {
        heartbeat_thread_->join();
    }
}

void RpcServer::Start(int option, int nEventLoopThread, int nTaskThreadNum /*= std::thread::hardware_concurrency()*/)
{
    server_.Start(option, nEventLoopThread, nTaskThreadNum);
}


void RpcServer::RegisterMethod(const std::string& strMethod, Handler handler)
{
    methods_[strMethod] = handler;
}


void RpcServer::OnMessage(const std::shared_ptr<TcpConnection>& con, std::string& strMsg)
{
    con->GetLoop()->AssertInLoopThread("RpcServer::OnMessage");

    //Buffer buf;
    //buf.Append(strMsg.data(), strMsg.size());

    uint64_t id;
    std::string strMethod, strParams;
    bool ok = RpcCodec::Protobuf_DecodeRequest(strMsg, id, strMethod, strParams);
    if(!ok)
    {
        this->HandlerResultResponse(con, id, eRpcCode_ParamError, "{}");
        return;
    }

    // 需要通过走rpcLogFile
    // file_ <<  "RpcServer::OnMessage id:="  << id << std::endl;
    auto itr = methods_.find(strMethod);
    if(itr == methods_.end())
    {
        this->HandlerResultResponse(con, id, eRpcCode_NotGetMethod, "{}");
        return;
    }

    std::string result;
    try{
        Handler handler = (itr->second);
        result = handler(strParams);
    }
    catch(const std::exception& e)
    {
        this->HandlerResultResponse(con, id, eRpcCode_ServerError, "{}");
        return;
    }

    this->HandlerResultResponse(con, id, eRpcCode_Success, std::move(result));
}


void RpcServer::HandlerResultResponse(const std::shared_ptr<TcpConnection>& con, uint64_t id, int32_t code, std::string strResult)
{
    
    std::string strResponse = std::move(RpcCodec::Protobuf_EncodeResponse(id, code, strResult));
    //std::cout << "Server sending response, size=" << strResponse.size() << " id=" << id << " code=" << code << std::endl;
    con->Send(strResponse);
}

       

void RpcServer::EnableServiceDiscovery(const std::string& registry_host, int registry_port, 
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
                //std::cout << "heartbeat succss!!!" << std::endl;
            }
        }
    });

    // 在定时的发心跳
}