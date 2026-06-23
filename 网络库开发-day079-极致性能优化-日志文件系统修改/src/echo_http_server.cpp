
#include "EventLoopThread.h"
#include "EventLoopThreadPool.h"
#include "EventLoop.h"
#include "TcpServer.h"
#include "Http/HttpServer.h"
#include "Common/ConfigManager.h"
#include "Common/JsonMethod.h"
#include "LibRpc/RpcServer.h"
#include "LibRpc/RpcConnectionPool.h"
#include "ServiceDiscovery/ServiceRegisterCenter.h"
#include "TcpClient.h"
#include <iostream>
#include <chrono>
#include <iostream>

#include <signal.h>
#include <thread>



int echo_http_server()
{
    
    signal(SIGPIPE, SIG_IGN);
    
    auto& cfg = ConfigManager::getInstance();
    if (!cfg.loadConfig("./config/server.ini")) {
        std::cerr << "Failed to load config\n";
        return -1;
    }

    int port = cfg.getInt("RegisterCenter", "port", 8080);
    EventLoop loop;
   
    // tcpServer
    /*
    TcpServer server(&loop, PORT);
    server.SetMessageCallBack(std::bind(&TcpServer::HandleOnMessage, 
        &server, std::placeholders::_1, 
        std::placeholders::_2)
    );
    server.Start(0, 6);
    */


    // httpSrver
    HttpServer server(&loop, port);
    server.RegisterMethod("add", JsonMethodLib::add);
    server.Start(); // eventloopThread 6个工作线程为最佳性能
    //ServiceRegisterCenter register_center(&server); // 注册中心 不需要的时候直接注释这一行就是普通的httpServer了

    loop.Loop();
    return 0;
}


