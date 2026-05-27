#pragma once

#include "../TcpServer.h"
#include <unordered_map>
#include <functional>
#include <string>


class EventLoop;
class RpcServer
{
public:
    using Handler = std::function<std::string(const std::string&)>;

    RpcServer(EventLoop* loop, int nPort);

    void Start(int option, int nEventLoopThread, int idleSecTimeOut, int nTaskThreadNum = std::thread::hardware_concurrency());

    void RegisterMethod(const std::string& strMethod, Handler handler);

    // 暂时测试使用
    //EventLoop* GetIndexLoop(int index) { return server_.GetIndexLoop(index); }

private:
    // 同步处理，不走任务线程池
    void OnMessage(const std::shared_ptr<TcpConnection>& con, std::string& strMsg);

    void HandlerResultResponse(const std::shared_ptr<TcpConnection>& con, uint32_t id, int32_t code, std::string strResult);

private:
    TcpServer server_;
    std::unordered_map<std::string, Handler> methods_;
};