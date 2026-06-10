
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



int main()
{
    
    signal(SIGPIPE, SIG_IGN);
    
    auto& cfg = ConfigManager::getInstance();
    if (!cfg.loadConfig("./config/server.ini")) {
        std::cerr << "Failed to load config\n";
        return -1;
    }


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
    int my_port = cfg.getInt("Rpc", "listen_port", 8888);
    RpcServer server(&loop, my_port);
    server.RegisterMethod("add", ProtoMethod::add);
    //server.RegisterMethod("echo", JsonMethodLib::echo);
    //server.RegisterMethod("login", JsonMethodLib::login);
    server.Start(0, 6);

    std::string my_ip = cfg.getString("Rpc", "server_ip", "127.0.0.1");
    std::string registry_host = cfg.getString("RegisterCenter", "ip", "127.0.0.1");
    int registry_port = cfg.getInt("RegisterCenter", "port", 8888);
    int ttl_sec = cfg.getInt("RegisterCenter", "ttl_sec", 30);

    server.EnableServiceDiscovery(registry_host, registry_port, "rpc_server", my_ip, my_port, ttl_sec);

    loop.Loop();
    rpcLog.Release();
    return 0;
}


