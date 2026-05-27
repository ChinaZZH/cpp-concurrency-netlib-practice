#include "RpcClient.h"
#include "RpcCodec.h"
#include "../TcpConnection.h"
#include "../Buffer.h"

RpcClient::RpcClient(TcpConnectionPtr con)
:con_(std::move(con))   // 暂时和deepseek提供的代码一致，使用std::move
{

}

// 发送请求，返回请求ID(不等待响应)
uint64_t RpcClient::SendRequest(const std::string& method, const std::string& params)
{
    uint64_t req_id = next_id_.fetch_add(1, std::memory_order_release);

    Buffer buf;
    RpcCodec::EncodeRequest(buf, req_id, method, params);
    
    std::string strData = buf.RetrieveAllAsString();
    con_->Send(strData);
    return req_id;
}