#include "RpcClient.h"
#include "RpcCodec.h"
#include "../TcpConnection.h"
#include "../Buffer.h"
#include "../EventLoop.h"
#include <stdexcept>
#include <iostream>
#include <inttypes.h>
#include "RpcLogFile.h"


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
std::pair<uint64_t, std::future<std::string>> RpcClient::SendRequest(const std::string& method, const std::string& params)
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

    uint64_t req_id = next_id_.fetch_add(2, std::memory_order_release);
    
    

    //Buffer buf;
     std::string strData = std::move(RpcCodec::Protobuf_EncodeRequest(req_id, method, params));
   //buf.RetrieveAllAsString();

    std::promise<std::string> pro;
    std::future<std::string> fut = pro.get_future();
    {
        std::lock_guard<std::mutex> lk(mutex_);
        pending_[req_id] = std::move(pro);

        /*
        uintptr_t client_addr = reinterpret_cast<uintptr_t>(this);
        std::string strRpcLog("RpcClient::Call add new client=");
        strRpcLog.append(std::to_string(client_addr));
        strRpcLog.append(" id:=");
        strRpcLog.append(std::to_string(req_id));
        strRpcLog.append(" fd:=");
        strRpcLog.append(std::to_string(con_->GetFd()));

        
        RpcLogFile& rpcLog = RpcLogFile::getInstance();
        rpcLog.AppendContent(strRpcLog);
        */
    }

    std::weak_ptr<TcpConnection> weakCon = con_->shared_from_this();
    event_loop->RunInLoop([event_loop, weakCon, strData = std::move(strData)](){
        auto conn = weakCon.lock();
        if(conn)
        {
            //std::cout << "RpcClient::SendRequest thread_id:=" << std::this_thread::get_id() << " connection thread_id:= " << event_loop->GetThreadId() << std::endl;
            conn->Send(strData);
        }
    });

   
    return std::pair<uint64_t, std::future<std::string>>(req_id, std::move(fut));
}


 // 同步调用， 阻塞直到收到响应或者超时
std::string RpcClient::Call(const std::string& method, const std::string& params, int timeout_ms /*= 5000*/)
{
    if(!bConnected_.load(std::memory_order_acquire))
    {
        throw std::runtime_error("RpcClient::Call Not connected to server");
    }


    auto reqInfo = SendRequest(method, params);
    uint64_t id = reqInfo.first;
    std::future<std::string> fut = std::move(reqInfo.second);
   
    //test_pending_time_.insert(std::make_pair(id, std::chrono::steady_clock::now()));
    auto status = fut.wait_for(std::chrono::milliseconds(timeout_ms));
    if(std::future_status::timeout == status)
    {
        {
            std::lock_guard<std::mutex> lk(mutex_);
            pending_.erase(id);
        }
        
        char error_log[32] = {0};
        snprintf(error_log, 31, "rpc timeout id:=%" PRIu64 " fd:=%d", id, con_->GetFd()); 
        throw std::runtime_error(error_log);
    }

    return fut.get();
}


// 处理响应(由网络消息回调调用)
void RpcClient::OnResponse(const std::string& data)
{
    
    //Buffer buf;
    //buf.Append(data);

    uint64_t res_id = 0;
    int32_t code = 0;
    std::string result;
    bool bDecodeReuslt = RpcCodec::Protobuf_DecodeResponse(data, res_id, code, result);
    if (!bDecodeReuslt)
    {
        // 解析失败，忽略
        return;
    }

    // 判断是奇数还是偶数，通过和0x01按位取与，如果结果为0则为偶数，否则为奇数
    uint64_t is_odd_num = (res_id & 0x01);
    if(0 != is_odd_num)
    {
         // 奇数，使用同步调用
        this->ProcessOnResponseByCall(res_id, code, result);
    }
    else{
       // 偶数，则使用异步调用
       this->ProcessOnResponseByAsyncCall(res_id, code, result);
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
        /*
        std::string strRpcLog("RpcClient::OncConnectionClosed rpc client:=");
        strRpcLog.append(std::to_string(reinterpret_cast<uintptr_t>(this)));
        strRpcLog.append("find id:=");
        strRpcLog.append(std::to_string(pair.first));
        strRpcLog.append(" fd:=");
        strRpcLog.append(std::to_string(con_->GetFd()));

        RpcLogFile& rpcLog = RpcLogFile::getInstance();
        rpcLog.AppendContent(strRpcLog);
        */

        auto exception_ptr = std::make_exception_ptr(std::runtime_error("connection_closed"));
        pair.second.set_exception(exception_ptr);
    }
}



void RpcClient::CallAsync(const std::string& method, const std::string& params, AsyncCallback cb)
{
    if(!bConnected_.load(std::memory_order_acquire))
    {
        throw std::runtime_error("RpcClient::CallAsync Not connected to server");
    }

    EventLoop* event_loop = con_->GetLoop();
    if(!event_loop)
    {
        throw std::runtime_error("RpcClient::SendRequest connection event_loop is nullptr");
    }

    uint64_t req_id = async_call_next_id_.fetch_add(2, std::memory_order_release);
    std::string strData = std::move(RpcCodec::Protobuf_EncodeRequest(req_id, method, params));
    
    {
        std::lock_guard<std::mutex> lk(aync_mutex_);
        async_callback_pending_func_[req_id] = cb;
    }


    std::weak_ptr<TcpConnection> weakCon = con_->shared_from_this();
    event_loop->RunInLoop([event_loop, weakCon, strData = std::move(strData)](){
        auto conn = weakCon.lock();
        if(conn)
        {
            //std::cout << "RpcClient::CallAsync thread_id:=" << std::this_thread::get_id() << " connection thread_id:= " << event_loop->GetThreadId() << std::endl;
            conn->Send(strData);
        }
    });

}


void RpcClient::ProcessOnResponseByCall(uint64_t res_id, int32_t code, const std::string& result)
{
    std::promise<std::string> p;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        auto itr = pending_.find(res_id);
        if(itr == pending_.end())
        {
            std::cout << "rpc Call Response pending_ no client:=" << reinterpret_cast<uintptr_t>(this) << "find id:=" << res_id << " fd:=" << con_->GetFd() << " code:=" << code << std::endl;
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


void RpcClient::ProcessOnResponseByAsyncCall(uint64_t res_id, int32_t code, const std::string& result)
{
    if(0 != code)
    {

    }

    bool bEmptyTaskList = true;
    AsyncCallback cb = nullptr;
    {
        std::lock_guard<std::mutex> lk(aync_mutex_);
        auto itr = async_callback_pending_func_.find(res_id);
        if(itr == async_callback_pending_func_.end())
        {
             std::cout << "rpc AsyncCall Response aync_callback_pending_func_ no client:=" << reinterpret_cast<uintptr_t>(this) << "find id:=" << res_id << " fd:=" << con_->GetFd() << " code:=" << code << std::endl;
            return;
        }

        cb = (itr->second);
        async_callback_pending_func_.erase(itr);
        bEmptyTaskList = async_callback_pending_func_.empty();
    }

    if(cb)
    {
        cb(result, code);
    }
}


