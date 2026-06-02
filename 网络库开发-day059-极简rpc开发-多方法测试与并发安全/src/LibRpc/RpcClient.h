#pragma once 

#include <memory>
#include <atomic>
#include <cstdint>
#include <string>
#include <future>
#include <mutex>
#include <unordered_map>
#include <nlohmann/json.hpp>

class TcpConnection;
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

class RpcClient: public std::enable_shared_from_this<RpcClient>
{
public:
    explicit RpcClient(TcpConnectionPtr con);

    void SetConnection(const TcpConnectionPtr& con);

    // 异步发送请求，返回请求ID(不等待响应)
    uint64_t SendRequest(const std::string& method, const std::string& params);

    // 同步调用， 阻塞直到收到响应或者超时
    std::string Call(const std::string& method, const std::string& params, int timeout_ms = 3000);

    // 模板化call的作用
    template<typename Req, typename Resp>
    Resp Call(const std::string& method, const Req& req, int timeout_ms = 3000)
    {
        std::string params = nlohmann::json(req).dump();
        std::string resp_str = Call(method, params, timeout_ms);
        return nlohmann::json::parse(resp_str).get<Resp>();
    }

    // 处理响应(由网络消息回调调用)
    void OnResponse(const std::string& data);

    // 连接关闭时调用(由TcpConnection的关闭回调触发)
    void OncConnectionClosed();

private:
    TcpConnectionPtr        con_;
    std::atomic<uint64_t>   next_id_ = 1;

    std::unordered_map<uint64_t, std::promise<std::string>> pending_;
    std::mutex mutex_;

    std::atomic<bool> bConnected_ = false; // 连接状态标志
};