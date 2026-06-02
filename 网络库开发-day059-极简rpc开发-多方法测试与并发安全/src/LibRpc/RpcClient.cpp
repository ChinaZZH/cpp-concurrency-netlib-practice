#include "RpcClient.h"
#include "RpcCodec.h"
#include "../TcpConnection.h"
#include "../Buffer.h"
#include "../EventLoop.h"
#include <stdexcept>
#include <iostream>

RpcClient::RpcClient(TcpConnectionPtr con)
{
    if(con)
    {
        SetConnection(con);
    }
}

void RpcClient::SetConnection(const TcpConnectionPtr& con)
{
    con_ = con;
    bConnected_.store(nullptr != con, std::memory_order_release);
    if (con_)
    {
        std::weak_ptr<RpcClient> weakClient = this->shared_from_this();
        con->SetCloseCallBack([weakClient](const TcpConnectionPtr&){
            auto rpcClient = weakClient.lock();
            if(rpcClient)
            {
                rpcClient->OncConnectionClosed();
            }
        });
    }
    
}


// 发送请求，返回请求ID(不等待响应)
uint64_t RpcClient::SendRequest(const std::string& method, const std::string& params)
{
    if(!bConnected_.load(std::memory_order_acquire))
    {
        throw std::runtime_error("RpcClient::SendRequest Not connected to server");
    }

    EventLoop* event_loop = con_->GetLoop();
    if(!event_loop)
    {
        throw std::runtime_error("RpcClient::SendRequest connection event_loop is nullptr");
    }

    uint64_t req_id = next_id_.fetch_add(1, std::memory_order_release);

    Buffer buf;
    RpcCodec::EncodeRequest(buf, req_id, method, params);
    
    std::string strData = buf.RetrieveAllAsString();
    std::weak_ptr<TcpConnection> weakCon = con_->shared_from_this();
    event_loop->RunInLoop([event_loop, weakCon, strData = std::move(strData)](){
        auto conn = weakCon.lock();
        if(conn)
        {
            //std::cout << "RpcClient::SendRequest thread_id:=" << std::this_thread::get_id() << " connection thread_id:= " << event_loop->GetThreadId() << std::endl;
            conn->Send(strData);
        }
    });

   
    return req_id;
}


 // 同步调用， 阻塞直到收到响应或者超时
std::string RpcClient::Call(const std::string& method, const std::string& params, int timeout_ms /*= 3000*/)
{
    if(!bConnected_.load(std::memory_order_acquire))
    {
        throw std::runtime_error("RpcClient::Call Not connected to server");
    }


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
    //std::cout << "RpcClient::OnResponse: received " << data.size() << " bytes" << std::endl;
    //std::cout << "RpcClient::OnResponse thread_id:=" << std::this_thread::get_id() << " connection thread_id:= " << con_->GetLoop()->GetThreadId() << std::endl;
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


void RpcClient::OncConnectionClosed()
{
    std::unordered_map<uint64_t, std::promise<std::string>> copy_pending;
    {
        std::lock_guard<std::mutex> lock(mutex_);
        pending_.swap(copy_pending);
    }


    bConnected_.store(false, std::memory_order_release);
    for(auto& pair: copy_pending)
    {
        auto exception_ptr = std::make_exception_ptr(std::runtime_error("connection_closed"));
        pair.second.set_exception(exception_ptr);
    }
}