#include "RpcClient.h"
#include "RpcCodec.h"
#include "../TcpConnection.h"
#include "../Buffer.h"
#include <stdexcept>
#include <iostream>

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


 // 同步调用， 阻塞直到收到响应或者超时
std::string RpcClient::Call(const std::string& method, const std::string& params, int timeout_ms /*= 3000*/)
{
    uint64_t id = SendRequest(method, params);
    std::promise<std::string> pro;
    std::future<std::string> fut = pro.get_future();
    {
        std::lock_guard<std::mutex> lk(mutex_);
        pending_[id] = std::move(pro);
    }

    auto status = fut.wait_for(std::chrono::milliseconds(timeout_ms));
    if(std::future_status::timeout == status)
    {
        std::lock_guard<std::mutex> lk(mutex_);
        pending_.erase(id);
        throw std::runtime_error("rpc timeout");
    }

    return fut.get();
}


// 处理响应(由网络消息回调调用)
void RpcClient::OnResponse(const std::string& data)
{
    std::cout << "RpcClient::OnResponse: received " << data.size() << " bytes" << std::endl;

    Buffer buf;
    buf.Append(data);

    uint32_t res_id = 0;
    int32_t code = 0;
    std::string result;
    bool bDecodeReuslt = RpcCodec::DecodeResponse(buf, res_id, code, result);
    if (!bDecodeReuslt)
    {
        // 解析失败，忽略
        return;
    }

    std::promise<std::string> p;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        auto itr = pending_.find(res_id);
        if(itr == pending_.end())
        {
            return;
        }

        p = std::move(itr->second);
        pending_.erase(itr);
    }
    
    if(eRpcCode_Success == code)
    {
        p.set_value(result);
    }
    else
    {
        auto exception_ptr = std::make_exception_ptr(std::runtime_error("rpc error code: " + std::to_string(code)));
        p.set_exception(exception_ptr);
    }
}