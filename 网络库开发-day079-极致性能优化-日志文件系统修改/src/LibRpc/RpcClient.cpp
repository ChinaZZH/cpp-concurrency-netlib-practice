#include "RpcClient.h"
#include "RpcCodec.h"
#include "RpcErrorCodeDef.h"
#include "../TcpConnection.h"
#include "../Buffer.h"
#include "../EventLoop.h"
#include "../EventLoopThread.h"
#include <stdexcept>
#include <iostream>
#include <inttypes.h>
#include <string>         // std::string
#include <fstream>        // std::ifstream
#include <functional>     // std::hash
#include <random>         // std::random_device
#include <unistd.h>       // gethostname, getpid
#include <cstdint>        // uint64_t
#include <sstream>        // std::ostringstream (如果用在其他函数)
#include <coroutine>
#include "../Decoder/LengthPrefixDecoder.h"
#include "../TcpClient.h"


RpcClient::RpcClient(TcpConnectionPtr con)
{
    if(con)
    {
        loop_ = con->GetLoop();
        SetConnection(con);
    }
}


RpcClient::RpcClient(const std::string& strIp, int nPort)
:ip_(strIp)
,port_(nPort)
{
    loop_thread_ = std::make_unique<EventLoopThread>();
    loop_ = loop_thread_->StartLoop();
    if(!loop_)
    {
        std::cerr << "RpcClient::RpcClient" << std::endl;
        exit(1);
    }
    
    tcp_client_ = std::make_shared<TcpClient>(loop_, strIp, nPort);
}


RpcClient::~RpcClient() = default;

// 连接池自动连接
bool RpcClient::AutoConnect()
{
    std::promise<void> con_promise;
    std::future<void> fut = con_promise.get_future();

    std::weak_ptr<RpcClient> weakRpcPtr = shared_from_this();
    tcp_client_->SetConnectionCallBack([&con_promise, weakRpcPtr](const TcpConnectionPtr& conn) {
        auto rpcClient = weakRpcPtr.lock();
        if(rpcClient)
        {
            //std::cout << "Connected to server" << std::endl;
            auto length_decoder = std::make_unique<LengthPrefixDecoder>();
            conn->SetDecoder(std::move(length_decoder));
            rpcClient->SetConnection(conn);
        }
        

        con_promise.set_value();
    });

    tcp_client_->SetMessageCallBack([weakRpcPtr](const TcpConnectionPtr&, std::string& msg) {
        auto rpcClient = weakRpcPtr.lock();
        if(rpcClient)
        {
             rpcClient->OnResponse(msg);
        }
    });


    tcp_client_->Connect();
    fut.wait();             // 等待建立连接， std::promise和std::future搭配使用，异步唤醒阻塞。
    return true;
}


void RpcClient::SetConnection(const TcpConnectionPtr& con)
{
    con_ = con;
    bConnected_.store(nullptr != con, std::memory_order_release);
    if (con_)
    {
        loop_ = con_->GetLoop();
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

    if(!loop_)
    {
        throw std::runtime_error("RpcClient::SendRequest connection event_loop is nullptr");
    }

    uint64_t req_id = next_id_.fetch_add(2, std::memory_order_release);
    std::string trace_id = RpcClient::GenerateTraceId();
    
    // 测试 链路追踪
    

    //Buffer buf;
    std::string strData = std::move(RpcCodec::Protobuf_EncodeRequest(req_id, method, params, trace_id));
   //buf.RetrieveAllAsString();

    std::promise<std::string> pro;
    std::future<std::string> fut = pro.get_future();
    {
        std::lock_guard<std::mutex> lk(mutex_);
        pending_[req_id] = std::move(pro);
    }

    std::weak_ptr<TcpConnection> weakCon = con_->shared_from_this();
    loop_->RunInLoop([weakCon, strData = std::move(strData)](){
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
    std::string trace_id;
    bool bDecodeReuslt = RpcCodec::Protobuf_DecodeResponse(data, res_id, code, result, trace_id);
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
    absl::flat_hash_map<uint64_t, std::promise<std::string>> copy_pending;
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



void RpcClient::CallAsync(const std::string& method, const std::string& params, AsyncCallback cb, int timeout_ms /*= 5000*/)
{
    if(!bConnected_.load(std::memory_order_acquire))
    {
        throw std::runtime_error("RpcClient::CallAsync Not connected to server");
    }

    if(!loop_)
    {
        throw std::runtime_error("RpcClient::SendRequest connection event_loop is nullptr");
    }

    uint64_t timer_id = ((timeout_ms <= 0)? 0 : loop_->GenerateNewTimerId());
    uint64_t req_id = async_call_next_id_.fetch_add(2, std::memory_order_release);
    std::string trace_id = RpcClient::GenerateTraceId();
    std::string strData = std::move(RpcCodec::Protobuf_EncodeRequest(req_id, method, params, trace_id));
    
    {
        std::lock_guard<std::mutex> lk(aync_mutex_);
        async_callback_pending_func_[req_id] = std::pair(cb, timer_id);
    }


    if(timeout_ms > 0)
    {
        // std::cout << "add timer_id:=" << timer_id << std::endl;
        int32_t fd = con_->GetFd();
        std::weak_ptr<RpcClient> weakRpcClient = shared_from_this();
        loop_->RunAfter(timer_id, std::chrono::milliseconds(timeout_ms), [timer_id, req_id, fd, weakRpcClient] () {
            // std::cout << "time_out timer_id=" << timer_id << std::endl;
            auto rpcClient = weakRpcClient.lock();
            if(rpcClient)
            {
                std::cout << "AsyncCall time_out rpc id:=" << req_id << std::endl;
                std::string msg_result;
                rpcClient->ProcessOnResponseByAsyncCall(req_id, eRpcCode_TimeOut, msg_result, true);
            }
        });
    }

    std::weak_ptr<TcpConnection> weakCon = con_->shared_from_this();
    loop_->RunInLoop([weakCon, strData = std::move(strData)](){
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


void RpcClient::ProcessOnResponseByAsyncCall(uint64_t res_id, int32_t code, const std::string& result, bool bTimeOut /*= false*/)
{
    AsyncCallback cb = nullptr;
    uint64_t timer_id = 0;
    {
        std::lock_guard<std::mutex> lk(aync_mutex_);
        auto itr = async_callback_pending_func_.find(res_id);
        if(itr == async_callback_pending_func_.end())
        {
            if(!bTimeOut)
            {
                std::cout << "rpc AsyncCall Response aync_callback_pending_func_ no client:=" << reinterpret_cast<uintptr_t>(this) << "find id:=" << res_id << " fd:=" << con_->GetFd() << " code:=" << code << std::endl;
            }
             
            return;
        }

        cb = (itr->second).first;
        timer_id = (itr->second).second;
        async_callback_pending_func_.erase(itr);
    }

    // 不是定时器到期触发 并且定时器id>0
    if(!bTimeOut && timer_id > 0 && loop_)
    {
        // std::cout << "cancel timer_id:=" << timer_id << std::endl;
        loop_->CancelTimer(timer_id);
    }

    if(cb)
    {
        cb(result, code);
    }
}

// 时间戳+现成标识+序列号
std::string RpcClient::GenerateTraceId()
{
    static std::atomic<uint64_t> seq{0};
    uint64_t seq_no = seq.fetch_add(1, std::memory_order_relaxed);

     // 使用 system_clock 获取毫秒级时间戳，便于跨进程排序
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    // 线程 ID 的哈希值（可直接转为字符串）
    std::stringstream ss;
    ss << std::this_thread::get_id();
    std::string tid_str = ss.str();  // 例如 "140234567890123"

    return std::to_string(now_ms) + "-" + tid_str + "-" + std::to_string(seq_no);
}


std::string RpcClient::GenerateGlobalTraceId() {
    static std::atomic<uint64_t> seq{0};
    uint64_t seq_no = seq.fetch_add(1, std::memory_order_relaxed);
    auto now_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    uint64_t machine = GetMachineId();

    std::ostringstream oss;
    oss << std::hex << machine << "-" << std::dec << now_ms << "-" << seq_no;
    return oss.str();
}


// 获取一个全局唯一的机器标识符（64位无符号整数）
uint64_t RpcClient::GetMachineId() {
    // 静态局部变量：函数首次调用时初始化，之后直接返回缓存值（线程安全，C++11起）
    static uint64_t machine_id = []() -> uint64_t {
        // 方案1：读取 Linux 系统机器ID文件（/etc/machine-id，通常由 systemd 生成，全球唯一）
        std::ifstream f("/etc/machine-id");  // 打开文件流
        std::string id;                     // 存储读取到的ID字符串
        // 如果文件状态良好、成功读取一行、且长度≥8（保证至少64位），则使用
        if (f.good() && std::getline(f, id) && id.size() >= 8) {
            // 计算字符串的哈希值，转换为64位整数（std::hash 对 string 的返回值是 size_t，足够64位）
            return std::hash<std::string>{}(id);
        }


        // 方案2（备选）：使用主机名 + 当前进程ID的组合作为机器标识
        char hostname[256];                // 存储主机名的缓冲区
        // gethostname 成功返回0，失败返回-1（需要包含 <unistd.h>）
        if (gethostname(hostname, sizeof(hostname)) == 0) {
            // 计算主机名的哈希值
            uint64_t h = std::hash<std::string>{}(hostname);
            // 将进程ID（通常32位）左移32位后异或到高位，避免与主机名哈希冲突
            h ^= static_cast<uint64_t>(getpid()) << 32;
            return h;
        }


        // 方案3（最终保底）：使用随机数生成器（std::random_device 提供真随机数）
        static std::random_device rd;       // 静态随机设备（线程安全，但可能阻塞）
        // 组合两个32位随机数构成64位值（一个随机数只有32位，组合后覆盖全部64位）
        return static_cast<uint64_t>(rd()) | (static_cast<uint64_t>(rd()) << 32);
    }();    // 注意：末尾的 () 表示立即执行 lambda 表达式

    
    return machine_id;
}

