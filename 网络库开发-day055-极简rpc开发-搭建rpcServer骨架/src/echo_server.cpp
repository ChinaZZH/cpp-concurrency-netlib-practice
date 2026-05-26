#include "EventLoopThread.h"
#include "EventLoopThreadPool.h"
#include "EventLoop.h"
#include "TcpServer.h"
#include "HttpServer.h"
#include "LibRpc/RpcServer.h"
#include <iostream>
#include <chrono>
#include <iostream>

#include <signal.h>
#include <thread>
#include <nlohmann/json.hpp>

#define PORT 8888

using json = nlohmann::json;
std::string add(const std::string& params)
{
    auto j = json::parse(params);
    int a =  j["a"];
    int b = j["b"];
    int result = a + b;

    json res = {{"result", result}};
    return res.dump();
}


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
    server.Start(0, 6, 0);

    loop.Loop();
    return 0;
}



