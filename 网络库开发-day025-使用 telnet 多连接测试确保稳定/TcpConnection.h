#pragma once

#include <memory>
#include <functional>
#include <string>

class EventLoop;
class ClientSocket;


class TcpConnection: public std::enable_shared_from_this<TcpConnection>
{
public:
    TcpConnection(EventLoop* loop, int fd);
    ~TcpConnection();

    void ConnectEstablished();  // 注册到EventLoop
    void Send(const std::string& strMessage);
    void Shutdown();

    EventLoop* GetLoop() const { return loop_; }
    int GetFd() const { return fd_; }

    using MessageCallBack = std::function<void(const std::shared_ptr<TcpConnection>&, std::string&)>;
    using CloseCallBack = std::function<void(const std::shared_ptr<TcpConnection>&)>;

    void SetMessageCallBack(MessageCallBack cb) { messageCallBack_ = cb; }
    void SetCloseCallBack(CloseCallBack cb) { closeCallBack_ = cb; }

private:
    void HandleRead();
    void HandleWrite();
    void HandleClose();
    void HandleError();

private:
    EventLoop* loop_;
    int fd_;
    std::unique_ptr<ClientSocket>  socket_;
    std::string inputBuffer_;       // 输入缓冲区
    std::string outputBuffer_;      // 输出缓冲区

    MessageCallBack messageCallBack_;
    CloseCallBack closeCallBack_;
};