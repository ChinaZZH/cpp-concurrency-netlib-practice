#include "HttpWebService.h"
#include "../TcpConnection.h"
#include "../EventLoop.h"
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/sendfile.h>
#include <unistd.h>
#include <sstream>
#include <iostream>
#include <filesystem>
#include <cassert>
#include <functional>


HttpWebService::HttpWebService() = default;


HttpWebService::~HttpWebService() = default;


// 注册静态文件路由
// url_prefix: 例如 "/static" 或 "/" (根路径)
// dir_path: 本地目录， 例如 "./www"
void HttpWebService::ServerStatic(const std::string& url_prefix, const std::string& dir_path)
{
    if(!std::filesystem::exists(dir_path)) {
        std::cerr << "Static directory not found: " << dir_path << std::endl;
        return;
    }

    // 将相对路径转换为绝对路径（基于当前工作目录）
    static_root_abs_ = std::move(std::filesystem::canonical(dir_path).string());
    // 注册路由...
}


// 发送静态文件(在业务回调中调用)
void HttpWebService::SendStaticFile(const std::string& filepath, const std::shared_ptr<TcpConnection>& con)
{
    // 检查文件是否存在
    if (!std::filesystem::exists(filepath)) {
        SendError(con, 404, "File Not Found");
        return;
    }

    // 安全检查: 防止路径遍历攻击
    std::string real_path;
    try {
        real_path = std::filesystem::canonical(filepath).string();
    } catch (const std::filesystem::filesystem_error& e) {
        SendError(con, 404, "Not Found");
        return;
    }

    if((0 != real_path.find(static_root_abs_))) // 仅仅允许指定目录
    {
        SendError(con, 403, "Forbidden");
        return;
    }

    int fd = ::open(real_path.c_str(), O_RDONLY);
    if(fd < 0)
    {
        SendError(con, 404, "Not Found");
        return;
    }

    // 对文件进行安全检查
    /*
        struct stat st;
        定义一个 stat 结构体，用于存储文件的元信息（如大小、权限、类型等）。

        ::fstat(fd, &st)
        通过文件描述符 fd 获取该文件的元信息，填充到 st 中。返回值：成功返回 0，失败返回 -1。

        S_ISREG(st.st_mode)
        检查文件类型是否是普通文件（regular file）。st_mode 是文件模式，S_ISREG 宏判断是否为普通文件。

        条件判断    
        如果 fstat 失败（例如 fd 无效）
        或者该文件不是普通文件（例如是目录、设备文件、符号链接等）
        则执行：
        关闭文件描述符 fd（避免资源泄露）。
        调用 SendError(conn, 403, "Forbidden") 发送 HTTP 403 错误（禁止访问）。
        返回，不再继续处理。
    */

    struct stat st;
    if(0 != ::fstat(fd, &st) || !S_ISREG(st.st_mode))
    {
        ::close(fd);
        SendError(con, 405, "file mode Forbidden");
        return;
    }

    // 构造响应头
    std::string mime = GetMimeType(real_path);
    std::ostringstream header;
    header << "HTTP/1.1 200 OK\r\n"
           << "Content-Type: " << mime << "\r\n"
           << "Content-Length: " << st.st_size << "\r\n"
           << "Connection: close\r\n"
           << "\r\n";

    con->Send(header.str());

    // 再通过sendfile发送文件内容(零拷贝)
    off_t offset = 0;
    ssize_t n;
    while(true)
    {  
        n = ::sendfile(con->GetFd(), fd, &offset, st.st_size - offset);
        if(n <= 0)
        {
            break;
        }
    }

    if(-1 == n && EAGAIN != errno)
    {
        // 错误处理， 可记录日志
    }

    ::close(fd);
}


// 传统 read+write 循环 进行性能比对
     /*
    char buf[8192];
    ssize_t n;
    ::lseek(fd, 0, SEEK_SET);
    while ((n = ::read(fd, buf, sizeof(buf))) > 0) {
        const char* p = buf;
        ssize_t left = n;
        while (left > 0) {
            ssize_t written = ::write(con->GetFd(), p, left);
            if (written <= 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // 非阻塞 socket 下，可稍后重试（简单起见，直接退出循环）
                    std::cout << "kernel full" << std::endl;
                }

                ::close(fd);
                return;
            }

            p += written;
            left -= written;
        }
    }
    */


// 发送静态文件(在业务回调中调用)
void HttpWebService::AysncSendStaticFile(const std::string& filepath, EventLoop* loop, const std::weak_ptr<TcpConnection>& weak_con)
{
    assert(loop);
    // 检查文件是否存在
    if (!std::filesystem::exists(filepath)) {
        AysncSendError(loop, weak_con, 404, "File Not Found");
        return;
    }

    // 安全检查: 防止路径遍历攻击
    std::string real_path;
    try {
        real_path = std::filesystem::canonical(filepath).string();
    } catch (const std::filesystem::filesystem_error& e) {
        AysncSendError(loop, weak_con, 404, "Not Found");
        return;
    }

    if((0 != real_path.find(static_root_abs_))) // 仅仅允许指定目录
    {
        AysncSendError(loop, weak_con, 403, "Forbidden");
        return;
    }

    int fd = ::open(real_path.c_str(), O_RDONLY);
    if(fd < 0)
    {
        AysncSendError(loop, weak_con, 404, "Not Found");
        return;
    }

    // 对文件进行安全检查
    /*
        struct stat st;
        定义一个 stat 结构体，用于存储文件的元信息（如大小、权限、类型等）。

        ::fstat(fd, &st)
        通过文件描述符 fd 获取该文件的元信息，填充到 st 中。返回值：成功返回 0，失败返回 -1。

        S_ISREG(st.st_mode)
        检查文件类型是否是普通文件（regular file）。st_mode 是文件模式，S_ISREG 宏判断是否为普通文件。

        条件判断    
        如果 fstat 失败（例如 fd 无效）
        或者该文件不是普通文件（例如是目录、设备文件、符号链接等）
        则执行：
        关闭文件描述符 fd（避免资源泄露）。
        调用 SendError(conn, 403, "Forbidden") 发送 HTTP 403 错误（禁止访问）。
        返回，不再继续处理。
    */

    struct stat st;
    if(0 != ::fstat(fd, &st) || !S_ISREG(st.st_mode))
    {
        ::close(fd);
        AysncSendError(loop, weak_con, 403, "Forbidden");
        return;
    }

    // 构造响应头
    std::string mime = GetMimeType(real_path);
    std::ostringstream header;
    header << "HTTP/1.1 200 OK\r\n"
           << "Content-Type: " << mime << "\r\n"
           << "Content-Length: " << st.st_size << "\r\n"
           << "Connection: close\r\n"
           << "\r\n";

    
    // 发送header
    std::string msg_header = header.str();

    loop->RunInLoop([fd, st, weak_con, msg_header=std::move(msg_header)](){
        auto con = weak_con.lock();
        if(!con)
        {
            return;
        }

        con->Send(msg_header);

        // 再通过sendfile发送文件内容
        off_t offset = 0;
        ssize_t n;
        while(true)
        {
            n = ::sendfile(con->GetFd(), fd, &offset, st.st_size - offset);
            if(n <= 0)
            {
                break;
            }
        }

        if(-1 == n && EAGAIN != errno)
        {
            // 错误处理， 可记录日志
        }

        ::close(fd);
    });
}

// 辅助 获取MIMIE类型
std::string HttpWebService::GetMimeType(const std::string& path)
{
    if (ends_with(path, ".html") || ends_with(path,".htm")) return "text/html";
    if (ends_with(path,".css")) return "text/css";
    if (ends_with(path,".js")) return "application/javascript";
    if (ends_with(path,".png")) return "image/png";
    if (ends_with(path,".jpg") || ends_with(path,".jpeg")) return "image/jpeg";
    if (ends_with(path,".txt")) return "text/plain";
    return "application/octet-stream";
}

// 辅助 发送错误响应
void HttpWebService::SendError(const std::shared_ptr<TcpConnection>& con, int code, const std::string& msg)
{
    std::string body = "<h1>" + std::to_string(code) + " " + msg + "</h1>";
    std::ostringstream response;
    response << "HTTP/1.1 " << code << " " << msg << "\r\n"
             << "Content-Type: text/html\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Connection: close\r\n"
             << "\r\n"
             << body;
    con->Send(response.str());
    con->Shutdown();  // 错误响应后关闭连接
}


void HttpWebService::AysncSendError(EventLoop* loop, const std::weak_ptr<TcpConnection>& weak_con, int code, const std::string& msg)
{
    assert(loop);
    std::string body = "<h1>" + std::to_string(code) + " " + msg + "</h1>";
    std::ostringstream response;
    response << "HTTP/1.1 " << code << " " << msg << "\r\n"
             << "Content-Type: text/html\r\n"
             << "Content-Length: " << body.size() << "\r\n"
             << "Connection: close\r\n"
             << "\r\n"
             << body;


    std::string result_msg = response.str();
    loop->RunInLoop([weak_con, result_msg = std::move(result_msg)](){
        auto con = weak_con.lock();
        if(con)
        {
            con->Send(result_msg);
            con->Shutdown();  // 错误响应后关闭连接
        }
    });
}

bool HttpWebService::ends_with(const std::string& str, const std::string& suffix) {
    if (str.size() < suffix.size()) return false;
    return str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

std::string HttpWebService::GetSystemFilePath(const std::string& file_path)
{
    return std::string(static_root_abs_ + file_path);
}