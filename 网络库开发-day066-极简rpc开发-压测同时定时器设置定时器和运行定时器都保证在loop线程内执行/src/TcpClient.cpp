
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
            loop_->DelayRemoveQueue(fd_, true);
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
    int socketFd = ClientSocket::Connect(ip_, port_);
    if(socketFd < 0)
    {
        std::cerr << "TcpClient::Connect connect error \n";
        return;
    }

    fd_ = socketFd;
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
    bConnecting = true;
    
    //std::cout << "TcpClient::Connect thread_id:" << std::this_thread::get_id() << " loop thread_id:" << loop_->GetThreadId() << std::endl;
}

 // 连接建立完成时的回调
void TcpClient::HandleWrite()
{
    //std::cout << "start TcpClient::HandleWrite thread_id " << std::this_thread::get_id() << " loop thread_id:" << loop_->GetThreadId() << std::endl;
    // 修改线程id, 之后数据io都走这个线程了
    loop_->DelEventToUpdateChannel(fd_, EPOLLOUT);
    std::weak_ptr<TcpClient> weakPtr = shared_from_this();
    loop_->DelayRemoveQueue(fd_, true, [weakPtr](){
        auto tcpClient = weakPtr.lock();
        if(tcpClient)
        {
            tcpClient->HandleNewConnection();
        }
    });
}


void TcpClient::HandleNewConnection()
{
    bConnecting = false;
    connection_ = std::make_shared<TcpConnection>(loop_, fd_, true);
    connection_->SetMessageCallBack(msgCb_);

    connection_->ConnectEstablished();

    if(connectionCb_)
    {
        connectionCb_(connection_);
    }

    //std::cout << "end TcpClient::HandleWrite thread_id:" << std::this_thread::get_id() << " loop thread_id:" << loop_->GetThreadId() << std::endl;
}