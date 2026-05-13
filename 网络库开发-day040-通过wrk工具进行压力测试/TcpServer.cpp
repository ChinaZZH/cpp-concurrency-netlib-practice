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

TcpServer::TcpServer(EventLoop* loop, int nPort, int nThreadNum /*= std::thread::hardware_concurrency() - 1*/)
:loop_(loop)
,port_(nPort)
,threadPool_(std::make_unique<ThreadPool>(std::max(1, nThreadNum)))
,messageCallBack_(nullptr)
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
    threadPool_->Start();

    // 将监听socket加入到epoll中去。
    std::unique_ptr<Channel> listenChannel = std::make_unique<Channel>(listenSocket_->GetSocketId());
    listenChannel->SetReadCallBack(std::bind(&TcpServer::HandleNewConnection, this));
    listenChannel->EnableReading();
    loop_->AddChannel(std::move(listenChannel));
}


void TcpServer::HandleNewConnection()
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
    if(messageCallBack_)
    {
        new_connection->SetMessageCallBack(messageCallBack_);
    }
    
    new_connection->SetCloseCallBack(std::bind(&TcpServer::RemoveConnection, this, std::placeholders::_1));
    new_connection->SetWaterMarkCallbacks(
        [](const std::shared_ptr<TcpConnection>& con){
            // 高水位回调：通知业务层暂停向该连接发送数据
            // 这里仅仅打印日志
            std::cout << "High water mark reached for fd=" << con->GetFd() << std::endl;
        },

        [](const std::shared_ptr<TcpConnection>& con){
            // 低水位回调：恢复正常发送
            // 这里仅仅打印日志
            std::cout << "Low water mark reached for fd=" << con->GetFd() << std::endl;
            con->WaterFromHighToLow();
        },

        64*1024,    // 高水位 64KB
        32*1024     // 低水位 32KB
    );

    new_connection->ConnectEstablished();
    mapTcpConnection_.insert(std::make_pair(nClientFd, new_connection));
}

void TcpServer::RemoveConnection(const std::shared_ptr<TcpConnection>& conn)
{
    std::cout << "Connection closed, fd=" << conn->GetFd() << std::endl;
    mapTcpConnection_.erase(conn->GetFd());
}


void TcpServer::HandleOnMessage(const std::shared_ptr<TcpConnection>& con, std::string& strMsg)
{
    // Echo 服务：原样发送回去
    //connection->Send(strMsg);
        
    // 将业务逻辑提交到线程池处理
    threadPool_->AddTask([con, strMsg](){
        // 模拟耗时业务（例如解析、数据库查询）
        std::cout << "start calculate" << std::endl;
        std::this_thread::sleep_for(std::chrono::seconds(5));
        std::cout << "end calculate" << std::endl;

        // 复杂的逻辑计算在这边
        // eventLoop->RunInLoop() 只会消息发送
        con->GetLoop()->RunInLoop([con, strMsg](){ con->Send(strMsg); });
    });
}