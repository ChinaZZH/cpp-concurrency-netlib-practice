
#include "EventLoopThread.h"
#include "EventLoopThreadPool.h"
#include "EventLoop.h"
#include "TcpServer.h"
#include "Http/HttpServer.h"
#include "Common/ConfigManager.h"
#include "Common/JsonMethod.h"
#include "Common/ProtoMethod.h"
#include "LibRpc/RpcServer.h"
#include "LibRpc/RpcLogFile.h"
#include "LibRpc/RpcConnectionPool.h"
#include "TcpClient.h"
#include <iostream>
#include <chrono>
#include <iostream>

#include <signal.h>
#include <thread>



int rpc_server_func()
{
    
    signal(SIGPIPE, SIG_IGN);
    
    auto& cfg = ConfigManager::getInstance();
    if (!cfg.loadConfig("./config/server.ini")) {
        std::cerr << "Failed to load config\n";
        return -1;
    }

    int port = cfg.getInt("Network", "port", 8888);
    int idle_ms_timeout = cfg.getInt("Connection", "idle_ms_timeout", 0);

    RpcLogFile& rpcLog = RpcLogFile::getInstance();
    rpcLog.OpenFile("rpc_server_log.txt");

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


   

    // rpcServer
    
    RpcServer server(&loop, port);
    server.RegisterMethod("add", ProtoMethod::add);
    //server.RegisterMethod("echo", JsonMethodLib::echo);
    //server.RegisterMethod("login", JsonMethodLib::login);
    server.Start(0, 6);

    loop.Loop();
    rpcLog.Release();
    return 0;
}


