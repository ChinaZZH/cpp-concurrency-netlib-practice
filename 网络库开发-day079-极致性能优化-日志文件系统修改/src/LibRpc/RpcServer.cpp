
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

void RpcServer::Start()
{
    server_.Start();
}


void RpcServer::RegisterMethod(const std::string& strMethod, Handler handler)
{
    methods_[strMethod] = handler;
}


void RpcServer::OnMessage(const std::shared_ptr<TcpConnection>& con, std::string& strMsg)
{
    EventLoop* loop_ptr = con->GetLoop();
    assert(loop_ptr);
    loop_ptr->AssertInLoopThread("RpcServer::OnMessage");

    //Buffer buf;
    //buf.Append(strMsg.data(), strMsg.size());
    ThreadPool* task_thread_pool = server_.GetThreadPool(); 
    assert(task_thread_pool);

    std::weak_ptr<TcpConnection> weak_connection_ptr = con;
    task_thread_pool->AddTask([this, loop_ptr, strMsg=std::move(strMsg), weak_connection_ptr](){
        if(!loop_ptr)
        {
            return;
        }

        uint64_t id;
        std::string strMethod, strParams, trace_id;
        bool ok = RpcCodec::Protobuf_DecodeRequest(strMsg, id, strMethod, strParams, trace_id);
        if(!ok)
        {
            this->HandlerResultResponse(loop_ptr, weak_connection_ptr, id, eRpcCode_ParamError, "{}", trace_id);
            return;
        }

        // 需要通过走rpcLogFile
        // file_ <<  "RpcServer::OnMessage id:="  << id << std::endl;
        auto itr = methods_.find(strMethod);
        if(itr == methods_.end())
        {
            this->HandlerResultResponse(loop_ptr, weak_connection_ptr, id, eRpcCode_NotGetMethod, "{}", trace_id);
            return;
        }


        std::string result;
        try{
            Handler handler = (itr->second);
            result = handler(strParams);
        }
        catch(const std::exception& e)
        {
            this->HandlerResultResponse(loop_ptr, weak_connection_ptr, id, eRpcCode_ServerError, "{}", trace_id);
            return;
        }

        this->HandlerResultResponse(loop_ptr, weak_connection_ptr, id, eRpcCode_Success, result, trace_id);
    });
}


void RpcServer::HandlerResultResponse(EventLoop* loop_ptr, const std::weak_ptr<TcpConnection>& weak_con, uint64_t id, int32_t code, const std::string& strResult, const std::string& trace_id)
{
    if(!loop_ptr)
    {
        return;
    }

    std::string strResponse = std::move(RpcCodec::Protobuf_EncodeResponse(id, code, strResult, trace_id));
    //std::cout << "Server sending response, size=" << strResponse.size() << " id=" << id << " code=" << code << std::endl;
    loop_ptr->RunInLoop([weak_con, strResponse=std::move(strResponse)](){
        auto con = weak_con.lock();
        if(con)
        {
            con->Send(strResponse);
        }
    });
}

       

void RpcServer::EnableServiceDiscovery(const std::string& registry_host, int registry_port, 
        const std::string& service_name, const std::string& my_ip, int my_port, int ttl_sec)
{
    if(service_registry_)
    {
        service_registry_->EnableServiceDiscovery(registry_host, registry_port, service_name, my_ip, my_port, ttl_sec);
    }
}