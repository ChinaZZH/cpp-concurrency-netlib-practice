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
:mainLoop_(loop)
,port_(nPort)
,taskThreadPool_(std::make_unique<ThreadPool>())
,messageCallBack_(nullptr)
,eventLoopThreadPool_(std::make_unique<EventLoopThreadPool>(loop, "TcpServer"))
{
    if(mainLoop_)
    {
        mainLoop_->InitServer(this);
    }

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

void TcpServer::Start(int option, int nEventLoopThread /*= std::thread::hardware_concurrency() - 1*/, int nTaskThreadNum /*= std::thread::hardware_concurrency() - 1*/)
{
    {
        nEventLoopThreadCount_ = std::max(1, nEventLoopThread);
        eventLoopThreadPool_->SetThreadNum(nEventLoopThreadCount_);
        eventLoopThreadPool_->Start(this);
    }

    {
        nTaskThreadNum = std::max(1, nTaskThreadNum);
        taskThreadPool_->Start(option, nTaskThreadNum);
    }
    

    std::cout << "event_loop_thread_pool thread_num:=" << nEventLoopThreadCount_ << " task_thread_pool thread_num:=" << nTaskThreadNum << std::endl;

    // 将监听socket加入到epoll中去。
    std::unique_ptr<Channel> listenChannel = std::make_unique<Channel>(listenSocket_->GetSocketId());
    listenChannel->SetReadCallBack(std::bind(&TcpServer::HandleNewConnection, this));
    listenChannel->EnableReading();
    mainLoop_->AddChannel(std::move(listenChannel));


    if(idleTimeOutSecs_ > 0)
    {
        mainLoop_->RunEvery(std::chrono::seconds(5), [this](){ this->CheckIdleConnections(); });
    }
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
    

    EventLoop* loop = eventLoopThreadPool_->getNextLoop();
    assert(loop);
    
    std::cout << "New connection fd=" << nClientFd 
              << " assigned to thread " << std::this_thread::get_id() 
              << " (ioLoop thread will be " << loop->GetThreadId() << ")" << std::endl;

    // 将新连接加入epoll  
    auto new_connection = std::make_shared<TcpConnection>(mainLoop_, loop, nClientFd);
    if(messageCallBack_)
    {
        new_connection->SetMessageCallBack(messageCallBack_);
    }
    
    new_connection->SetCloseCallBack(std::bind(&TcpServer::ClosedConnection, this, std::placeholders::_1));
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

    // 跟channel相关投递到connection所负债均衡选择的eventloop线程
    loop->RunInLoop([new_connection](){
        new_connection->ConnectEstablished();
    });
    
    mapTcpConnection_.insert(std::make_pair(nClientFd, new_connection));
}

void TcpServer::RemoveConnectionByFd(int fd)
{
    std::cout <<  "Connection closed, fd=" << fd << std::endl;
    mapTcpConnection_.erase(fd);
}

void TcpServer::ClosedConnection(const std::shared_ptr<TcpConnection>& conn)
{
    // 移除mapTcpConnection_的任务交给EventLoop以保证顺序的正确性。
    // mapTcpConnection_.erase(conn->GetFd());
    conn->ClosedConnection();
}


void TcpServer::HandleOnMessage(const std::shared_ptr<TcpConnection>& con, std::string& strMsg)
{
    // Echo 服务：原样发送回去
    // std::this_thread::sleep_for(std::chrono::seconds(1));
    std::cout << "TcpServer::HandleOnMessage msg:=" << strMsg.c_str() << std::endl; 
    con->Send(strMsg);

    // 将业务逻辑提交到线程池处理
    /*
    taskThreadPool_->AddTask([con, strMsg](){
            // 模拟耗时业务（例如解析、数据库查询）
            //  测试定时器的使用使用
            
            auto micros = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
            std::cout << "Start Delay start:=" <<  micros << std::endl;
            std::chrono::seconds delay(5);


            con->GetLoop()->RunAfter(delay, [con, strMsg]() {
                auto micros = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
                std::cout << "End Delay end:=" << micros << std::endl; 

                con->GetLoop()->RunInLoop([con, strMsg](){ con->Send(strMsg); });
            

            // 模拟耗时业务（例如解析、数据库查询）
            std::this_thread::sleep_for(std::chrono::seconds(5));

            // 复杂的逻辑计算在这边
            // eventLoop->RunInLoop() 只会消息发送
            con->GetLoop()->RunInLoop([con, strMsg](){ con->Send(strMsg); });
        });
        */
}



void TcpServer::SetConnectionIdleTimeOut(int nSecs)
{
    idleTimeOutSecs_ = nSecs;
}


void TcpServer::CheckIdleConnections()
{
    if(idleTimeOutSecs_ <= 0)
    {
        return ;
    }

    for(auto itr = mapTcpConnection_.begin(); itr != mapTcpConnection_.end(); ++itr)
    {
        auto& con = (itr->second);
        if(!con)
        {
            continue;
        }

        EventLoop* loop = con->GetLoop();
        if(!loop)
        {
            continue;
        }

        int timeOutIdleSecs = idleTimeOutSecs_;
        std::weak_ptr<TcpConnection> weakConPtr = con;
        loop->RunInLoop([weakConPtr, timeOutIdleSecs](){
            auto con = weakConPtr.lock();
            if(!con)
            {
                return ;
            }

            if(con->IsWriteClosed())
            {
                return ;
            }
        
            auto now = std::chrono::steady_clock::now();
            auto idleDuration = std::chrono::duration_cast<std::chrono::seconds>(now - con->GetLastActiveTime()); 
            if (idleDuration.count() >= timeOutIdleSecs)
            {
                //auto now_secs = std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()).count();
                //std::cout << "TcpServer::CheckIdleConnections now:=" <<  now_secs << std::endl;
                con->Shutdown();
            }
        });

        
    }
}