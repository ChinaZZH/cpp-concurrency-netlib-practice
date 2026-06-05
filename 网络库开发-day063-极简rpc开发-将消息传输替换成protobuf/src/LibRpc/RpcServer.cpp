
#include "RpcServer.h"
#include "RpcCodec.h"
#include "../TcpConnection.h"
#include "../EventLoop.h"
#include "../Decoder/LengthPrefixDecoder.h"
#include <memory>

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



void RpcServer::Start(int option, int nEventLoopThread, int idleSecTimeOut, int nTaskThreadNum /*= std::thread::hardware_concurrency()*/)
{
    server_.Start(option, nEventLoopThread, idleSecTimeOut, nTaskThreadNum);
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
    Buffer resBuf;
    RpcCodec::Protobuf_EncodeResponse(resBuf, id, code, strResult);

    std::string strResponse = resBuf.RetrieveAllAsString();
    //std::cout << "Server sending response, size=" << strResponse.size() << " id=" << id << " code=" << code << std::endl;
    con->Send(strResponse);
}

       