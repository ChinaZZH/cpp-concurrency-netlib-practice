#include "TcpServer.h"

#include "ListenSocket.h"
#include "ClientSocket.h"
#include "Epoll.h"
#include "Channel.h"
#include "EventLoop.h"
#include "TcpConnection.h"
#include <iostream>
#include <cstring>
#include <unistd.h>

TcpServer::TcpServer(EventLoop* loop, int nPort)
:loop_(loop)
,port_(nPort)
{
   // 1.创建socket
    listenSocket_ = std::make_unique<ListenSocket>();
    if(!listenSocket_->IsValid())
    {
        std::cerr << "listen_Socket is invalid " << std::endl;;
        exit(1);
    }

    // 2.设置地址重用
    
    listenSocket_->SetReuseAddr();

    // 3.绑定地址
    InetAddress addr(port_);
    if(false == listenSocket_->Bind(addr))
    {
         std::cerr << "listen_Socket bind error " << std::endl;;
          exit(1);
    }


    // 4.监听
    if(false == listenSocket_->Listen())
    {
        std::cerr << "listen_Socket listen error  " << std::endl;;
         exit(1);
    }

   
    
}
    
TcpServer::~TcpServer()
{

}

void TcpServer::Start()
{
    // 将监听socket加入到epoll中去。
    std::unique_ptr<Channel> listenChannel = std::make_unique<Channel>(listenSocket_->GetSocketId());
    listenChannel->SetReadCallBack(std::bind(&TcpServer::HadleNewConnection, this));
    listenChannel->EnableReading();
    loop_->AddChannel(std::move(listenChannel));
}


void TcpServer::HadleNewConnection()
{
    int nClientFd = listenSocket_->Accept();
    if(nClientFd < 0)
    {
        std::cerr << "accept error \n";
        return;
    }


    {
        // 已经存在该对象了
        auto itr = mapTcpConnection_.find(nClientFd);
        if(itr != mapTcpConnection_.end())
        {
            return ;
        }
    }
    

    // 将新连接加入epoll  
    auto new_connection = std::make_shared<TcpConnection>(loop_, nClientFd);
    new_connection->SetMessageCallBack([](const std::shared_ptr<TcpConnection>& connection, std::string& strMsg){
        // Echo 服务：原样发送回去
        connection->Send(strMsg);
    });

    new_connection->SetCloseCallBack(std::bind(&TcpServer::RemoveConnection, this, std::placeholders::_1));

    new_connection->ConnectEstablished();
    mapTcpConnection_.insert(std::make_pair(nClientFd, new_connection));
}

void TcpServer::RemoveConnection(const std::shared_ptr<TcpConnection>& conn)
{
    std::cout << "Connection closed, fd=" << conn->GetFd() << std::endl;
    mapTcpConnection_.erase(conn->GetFd());
}