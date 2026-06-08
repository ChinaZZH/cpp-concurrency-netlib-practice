#include "TcpServer.h"

#include "ListenSocket.h"
#include "ClientSocket.h"
#include "Epoll.h"
#include "Channel.h"
#include "EventLoop.h"
#include "TcpConnection.h"
#include "Decoder/LineDecoder.h"
#include <iostream>
#include <cstring>
#include <unistd.h>

TcpServer::TcpServer(EventLoop* loop, int nPort)
:mainLoop_(loop)
,port_(nPort)
,taskThreadPool_(std::make_unique<ThreadPool>())
,eventLoopThreadPool_(std::make_unique<EventLoopThreadPool>(loop, "TcpServer"))
,messageCallBack_(nullptr)
,closeCallBack_(nullptr)
,connectionCallBack_(nullptr)
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

    // 默认使用line分隔，或者默认不设置不分割。这里面先设置默认使用line分隔
    this->SetConnectionCallBack([](const std::shared_ptr<TcpConnection>& con){
        auto line_decoder = std::make_unique<LineDecoder>();
        con->SetDecoder(std::move(line_decoder));
    });
}
    
TcpServer::~TcpServer()
{
    
}

void TcpServer::Start(int option, int nEventLoopThread, int idleSecTimeOut, int nTaskThreadNum /*= std::thread::hardware_concurrency()*/)
{
    //std::cout << "TcpServer::Start 1111" << std::endl;
    {
        nEventLoopThreadCount_ = std::max(0, nEventLoopThread);
        eventLoopThreadPool_->SetThreadNum(nEventLoopThreadCount_);
        eventLoopThreadPool_->Start(this, idleSecTimeOut);
    }
//std::cout << "TcpServer::Start 222" << std::endl;
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
        auto itr = mapLightClient.find(nClientFd);
        if(itr != mapLightClient.end())
        {
            return ;
        }
    }
    

    EventLoop* loop = eventLoopThreadPool_->getNextLoop();
    assert(loop);

    // 跟channel相关投递到connection所负债均衡选择的eventloop线程
    auto newConnection = std::make_shared<TcpConnection>(loop, nClientFd);
    if(messageCallBack_)
    {
        newConnection->SetMessageCallBack(messageCallBack_);
    }

    if(connectionCallBack_)
    {
        newConnection->SetConnectionCallBack(connectionCallBack_);
    }

    loop->RunInLoop([loop, newConnection](){
        if(loop){
            loop->HandleNewConnection(newConnection);
        }
    });

    mapLightClient.insert(std::make_pair(nClientFd, loop));
}


void TcpServer::RemoveClinetFd(int fd)
{
    mapLightClient.erase(fd);
}


void TcpServer::HandleOnMessage(const std::shared_ptr<TcpConnection>& con, std::string& strMsg)
{
    // Echo 服务：原样发送回去
    // std::this_thread::sleep_for(std::chrono::seconds(1));
    //std::cout << "TcpServer::HandleOnMessage msg:=" << strMsg.c_str() << std::endl; 
    con->GetLoop()->AssertInLoopThread("TcpServer::HandleOnMessage");
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





