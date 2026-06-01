/*
#pragma once

#include <memory>
#include <functional>
#include <string>


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

private:
    void HandleWrite();     // 连接建立完成时的回调

    void NewConnection(int socket_fd);

private:
    EventLoop* loop_;
    std::string ip_;
    int port_;
    int socketFd_;
    std::unique_ptr<Channel> channel_;
    TcpConnectionPtr connection_;

    ConnectionCallBack  connectionCb_;
    MessageCallBack     msgCb_;     
};
*/

#include "TcpClient.h"
#include "ClientSocket.h"
#include "EventLoop.h"
#include "Channel.h"
#include "TcpConnection.h"
#include <iostream>
#include <cstring>

TcpClient::TcpClient(EventLoop* loop, const std::string& strIp, int nPort)
:loop_(loop)
,ip_(strIp)
,port_(nPort)
,fd_(-1)
,bConnecting(false)
{

}

TcpClient::~TcpClient()
{
    if(fd_ >= 0)
    {
        if(bConnecting)
        {
            // 尚未连接成功，直接移除等待 Channel 并关闭 socket
            loop_->NowToRemoveChannel(fd_, EventLoop::RemoveChannelNowToken());
        }
        else if(connection_ && connection_->IsValid())
        {
            // 连接已建立，需要通知 EventLoop 线程关闭连接
            std::weak_ptr<TcpConnection> weakConn = connection_;
            loop_->RunInLoop([weakConn]() {
                auto conn = weakConn.lock();
                if (conn && conn->IsValid()) 
                {
                    conn->Shutdown();   // 优雅关闭，最终会触发 HandleClose 并从 EventLoop 移除
                }
            });
        }
    }
}

void TcpClient::Connect()
{
    // 创建socekt并且连接
    {
        int socketFd = socket(AF_INET, SOCK_STREAM, 0);
        if (socketFd < 0)
        {
            std::cerr << "TcpClient::Connect socket error \n";
            return ;
        }
    
        int flags = ::fcntl(socketFd, F_GETFL, 0);
        flags |= O_NONBLOCK;
        ::fcntl(socketFd, F_SETFL, flags);
    
        InetAddress addr(ip_, port_);
        int result = ::connect(socketFd, (struct sockaddr*)addr.GetSockAddr(), sizeof(addr));
        if(result < 0 && errno != EINPROGRESS)
        {
            close(socketFd);
            std::cerr << "ClientSocket connect error \n";
            return ;
        }

        fd_ = socketFd;
    }
    
    auto channel = std::make_unique<Channel>(fd_);
    std::weak_ptr<TcpClient> weakClient = shared_from_this();
    channel->SetWriteCallBack([weakClient](){
        auto tcpClient = weakClient.lock();
        if(tcpClient)
        {
            tcpClient->HandleWrite();
        }
    });
    
    channel->EnableWriting();       // 用来检测非阻塞 connect 是否完成。 连接完成后，再注册读事件 (EPOLLIN)
    loop_->AddChannel(std::move(channel), "TcpClient::Connect");
    channel.reset();
    bConnecting = true;
}

 // 连接建立完成时的回调
void TcpClient::HandleWrite()
{
    //loop_->DelEventToUpdateChannel(clientSocket_->GetSocketFd(), EPOLLOUT);
    loop_->NowToRemoveChannel(fd_, EventLoop::RemoveChannelNowToken());
    bConnecting = false;

    connection_ = std::make_shared<TcpConnection>(loop_, fd_, true);
    connection_->SetMessageCallBack(msgCb_);
    connection_->ConnectEstablished();

    if(connectionCb_)
    {
        connectionCb_(connection_);
    }
}

