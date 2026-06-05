#pragma once

#include <memory>
#include <functional>
#include <string>


class ClientSocket;
class Channel;
class EventLoop;
class TcpConnection;
using TcpConnectionPtr = std::shared_ptr<TcpConnection>;

class TcpClient: public std::enable_shared_from_this<TcpClient>
{
public:
    using ConnectionCallBack = std::function<void(const TcpConnectionPtr&)>;
    using MessageCallBack = std::function<void(const TcpConnectionPtr&, std::string&)>;
   
    TcpClient(EventLoop* loop, const std::string& strIp, int nPort);

    ~TcpClient();

    void Connect();

    void SetConnectionCallBack(ConnectionCallBack cb) { connectionCb_ = cb; }

    void SetMessageCallBack(MessageCallBack cb) { msgCb_ = cb; }

    void HandleWrite();     // 连接建立完成时的回调

    void HandleNewConnection();

private:
    EventLoop* loop_;
    std::string ip_;
    int port_;
    
    int fd_;
    TcpConnectionPtr connection_;

    ConnectionCallBack  connectionCb_;
    MessageCallBack     msgCb_;     
    bool bConnecting = false;
};