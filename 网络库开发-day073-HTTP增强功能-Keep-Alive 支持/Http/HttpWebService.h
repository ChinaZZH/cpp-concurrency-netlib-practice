#pragma once

#include <string>
#include <memory>
/*
#include <memory>
#include <thread>
#include "HttpContext.h"
#include "../TcpServer.h"


class EventLoop;
*/

class TcpConnection;
class EventLoop;

class HttpWebService
{
public:
    HttpWebService();

    ~HttpWebService();

public:
    // 注册静态文件路由
    // url_prefix: 例如 "/static" 或 "/" (根路径)
    // dir_path: 本地目录， 例如 "./www"
    void ServerStatic(const std::string& url_prefix, const std::string& dir_path);

    // 发送静态文件(在业务回调中调用)
    void SendStaticFile(const std::string& filepath, const std::shared_ptr<TcpConnection>& con);


    std::string GetSystemFilePath(const std::string& file_path);

private:
    // 辅助 获取MIMIE类型
    std::string GetMimeType(const std::string& path);

    // 辅助 发送错误响应
    void SendError(const std::shared_ptr<TcpConnection>& con, int code, const std::string& msg);

    bool ends_with(const std::string& str, const std::string& suffix);

    // 处理在任务线程池处理的时候禁用，不提供(短连接不合适)
    // 发送静态文件(在业务回调中调用)
    void AysncSendStaticFile(const std::string& filepath, EventLoop* loop, const std::weak_ptr<TcpConnection>& weak_con);

    void AysncSendError(EventLoop* loop, const std::weak_ptr<TcpConnection>& weak_con, int code, const std::string& msg);

private:
    std::string static_root_abs_;
};