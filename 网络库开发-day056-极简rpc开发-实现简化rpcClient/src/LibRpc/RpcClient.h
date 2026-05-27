#pragma once 

#include <memory>
#include <atomic>
#include <cstdint>
#include <string>

class TcpConnection;
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

class RpcClient
{
public:
    explicit RpcClient(TcpConnectionPtr con);

    // 发送请求，返回请求ID(不等待响应)
    uint64_t SendRequest(const std::string& method, const std::string& params);

private:
    TcpConnectionPtr        con_;
    std::atomic<uint64_t>   next_id_ = 1;
};