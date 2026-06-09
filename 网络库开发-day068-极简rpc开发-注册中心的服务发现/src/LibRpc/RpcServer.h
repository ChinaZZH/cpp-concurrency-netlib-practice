#pragma once

#include "../TcpServer.h"
#include <unordered_map>
#include <functional>
#include <string>
#include <fstream>


class EventLoop;
class RpcServer
{
public:
    using Handler = std::function<std::string(const std::string&)>;

    RpcServer(EventLoop* loop, int nPort);

    ~RpcServer() = default;

    void Start(int option, int nEventLoopThread, int nTaskThreadNum = std::thread::hardware_concurrency());

    void RegisterMethod(const std::string& strMethod, Handler handler);

private:
    // 同步处理，不走任务线程池
    void OnMessage(const std::shared_ptr<TcpConnection>& con, std::string& strMsg);

    void HandlerResultResponse(const std::shared_ptr<TcpConnection>& con, uint64_t id, int32_t code, std::string strResult);

private:
    TcpServer server_;
    std::unordered_map<std::string, Handler> methods_;
};