#include "EventLoopThread.h"
#include "EventLoopThreadPool.h"
#include "EventLoop.h"
#include "TcpServer.h"
#include "HttpServer.h"
#include "LibRpc/RpcServer.h"
#include "LibRpc/RpcClient.h"
#include "LibRpc/RpcTestClientFile.h"
#include "TcpClient.h"
#include <iostream>
#include <chrono>
#include <iostream>

#include <signal.h>
#include <thread>
#include <nlohmann/json.hpp>

#define PORT 8888


int main()
{
    
    signal(SIGPIPE, SIG_IGN);
    
    EventLoop loop;
   
    // tcpServer
    /*
    TcpServer server(&loop, PORT);
    server.SetMessageCallBack(std::bind(&TcpServer::HandleOnMessage, 
        &server, std::placeholders::_1, 
        std::placeholders::_2)
    );
    server.Start(0, 6, 5);
    */


    // httpSrver
    //HttpServer server(&loop, PORT);
    //server.Start(0, 6, 60); // eventloopThread 6个工作线程为最佳性能

    // rpcServer
    RpcServer server(&loop, PORT);
    server.RegisterMethod("add", add);
    server.RegisterMethod("echo", echo);
    server.RegisterMethod("login", login);

    server.Start(0, 6, 0);
    //EventLoop* pLoop = server.GetIndexLoop(5);
    //pLoop->RunAfter(std::chrono::seconds(5), client_function);

    loop.Loop();
    return 0;
}


int multiThreadTestFunction()
{
    std::vector<std::thread> vecThreadList;
    for(int i = 0; i < 10; ++i)
    {
        vecThreadList.emplace_back(client_function, i*100);
    }


    for(auto& th : vecThreadList)
    {
        th.join();
    }

    return 0;
}


