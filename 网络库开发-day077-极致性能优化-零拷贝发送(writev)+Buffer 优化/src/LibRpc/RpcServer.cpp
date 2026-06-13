
#include "RpcServer.h"
#include "RpcCodec.h"
#include "RpcErrorCodeDef.h"
#include "../TcpConnection.h"
#include "../EventLoop.h"
#include "../Decoder/LengthPrefixDecoder.h"
#include "../Http/SimpleHttpClient.h"
#include "../ServiceDiscovery/ServiceRegistry.h"
#include <memory>
#include <nlohmann/json.hpp>

RpcServer::RpcServer(EventLoop* loop, int nPort)
:server_(loop, nPort)
,service_registry_(std::make_unique<ServiceRegistry>())
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
    std::string strMethod, strParams, trace_id;
    bool ok = RpcCodec::Protobuf_DecodeRequest(strMsg, id, strMethod, strParams, trace_id);
    if(!ok)
    {
        this->HandlerResultResponse(con, id, eRpcCode_ParamError, "{}", trace_id);
        return;
    }

    // 需要通过走rpcLogFile
    // file_ <<  "RpcServer::OnMessage id:="  << id << std::endl;
    auto itr = methods_.find(strMethod);
    if(itr == methods_.end())
    {
        this->HandlerResultResponse(con, id, eRpcCode_NotGetMethod, "{}", trace_id);
        return;
    }

    std::string result;
    try{
        Handler handler = (itr->second);
        result = handler(strParams);
    }
    catch(const std::exception& e)
    {
        this->HandlerResultResponse(con, id, eRpcCode_ServerError, "{}", trace_id);
        return;
    }

    this->HandlerResultResponse(con, id, eRpcCode_Success, result, trace_id);
}


void RpcServer::HandlerResultResponse(const std::shared_ptr<TcpConnection>& con, uint64_t id, int32_t code, const std::string& strResult, const std::string& trace_id)
{
    
    std::string strResponse = std::move(RpcCodec::Protobuf_EncodeResponse(id, code, strResult, trace_id));
    //std::cout << "Server sending response, size=" << strResponse.size() << " id=" << id << " code=" << code << std::endl;
    con->Send(strResponse);
}

       

void RpcServer::EnableServiceDiscovery(const std::string& registry_host, int registry_port, 
        const std::string& service_name, const std::string& my_ip, int my_port, int ttl_sec)
{
    if(service_registry_)
    {
        service_registry_->EnableServiceDiscovery(registry_host, registry_port, service_name, my_ip, my_port, ttl_sec);
    }
}