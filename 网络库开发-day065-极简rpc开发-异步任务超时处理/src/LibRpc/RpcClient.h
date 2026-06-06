#pragma once 

#include <memory>
#include <atomic>
#include <cstdint>
#include <string>
#include <future>
#include <mutex>
#include <unordered_map>
#include <nlohmann/json.hpp>
#include <condition_variable>


class TcpConnection;
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

class RpcClient: public std::enable_shared_from_this<RpcClient>
{
public:
    using AsyncCallback = std::function<void(const std::string& result, int32_t error)>;

    explicit RpcClient(TcpConnectionPtr con);

    void SetConnection(const TcpConnectionPtr& con);


    // 同步调用， 阻塞直到收到响应或者超时
    std::string Call(const std::string& method, const std::string& params, int timeout_ms = 5000);

    // 模板化call的作用
    template<typename Req, typename Resp>
    Resp Call(const std::string& method, const Req& req, int timeout_ms = 5000)
    {
        std::string params = nlohmann::json(req).dump();
        std::string resp_str = Call(method, params, timeout_ms);
        return nlohmann::json::parse(resp_str).get<Resp>();
    }

    // 处理响应(由网络消息回调调用)
    void OnResponse(const std::string& data);

    // 连接关闭时调用(由TcpConnection的关闭回调触发)
    void OncConnectionClosed();

public:
    void CallAsync(const std::string& method, const std::string& params, AsyncCallback cb);

    void ProcessOnResponseByCall(uint64_t res_id, int32_t code, const std::string& result);

    void ProcessOnResponseByAsyncCall(uint64_t res_id, int32_t code, const std::string& result);

    void WaitAsyncTaskCompleted();

private:
    std::pair<uint64_t, std::future<std::string>> SendRequest(const std::string& method, const std::string& params);

private:
    TcpConnectionPtr        con_;
    std::atomic<uint64_t>   next_id_ = 1; // 奇数的id让同步Call使用

    std::unordered_map<uint64_t, std::promise<std::string>> pending_;
    std::mutex mutex_;

    std::atomic<bool> bConnected_ = false; // 连接状态标志

    // 测试使用
    //std::unordered_map<uint64_t, std::chrono::time_point<std::chrono::steady_clock>> test_pending_time_;

    std::atomic<uint64_t>   async_call_next_id_ = 2; // 偶数的id让异步AsyncCall使用
    std::map<uint64_t, AsyncCallback> async_callback_pending_func_;
    std::mutex aync_mutex_;
};