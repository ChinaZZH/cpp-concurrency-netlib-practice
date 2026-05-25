#include "EventLoopThread.h"
#include "EventLoopThreadPool.h"
#include "EventLoop.h"
#include "TcpServer.h"
#include "HttpServer.h"
#include <iostream>
#include <chrono>
#include <iostream>

#include <signal.h>
#include <thread>

#define PORT 8888


int main()
{
    
    signal(SIGPIPE, SIG_IGN);
    
    EventLoop loop;
    /*
    TcpServer server(&loop, PORT);
    server.SetMessageCallBack(std::bind(&TcpServer::HandleOnMessage, 
        &server, std::placeholders::_1, 
        std::placeholders::_2)
    );
    server.Start(0, 6);
    */

    // httpSrver
    HttpServer server(&loop, PORT);
    server.Start(0, 6); // eventloopThread 6个工作线程为最佳性能
    loop.Loop();
    return 0;
}



