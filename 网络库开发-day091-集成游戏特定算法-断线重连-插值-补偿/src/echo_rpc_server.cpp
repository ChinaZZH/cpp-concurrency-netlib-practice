
#include "EventLoopThread.h"
#include "EventLoopThreadPool.h"
#include "EventLoop.h"
#include "TcpServer.h"
#include "Http/HttpServer.h"
#include "Common/ConfigManager.h"
#include "Common/JsonMethod.h"
#include "Common/ProtoMethod.h"
#include "LibRpc/RpcServer.h"
#include "LibRpc/RpcConnectionPool.h"
#include "GameSpecficAlgorithms/GameTest/AlgorithmsUnitTesting.h"
#include "TcpClient.h"
#include <iostream>
#include <chrono>
#include <iostream>

#include "GameSpecficAlgorithms/GameServer.h"
#include "GameSpecficAlgorithms/AOI/AOIPerformanceTest.h"
#include "GameSpecficAlgorithms/GameTest/Test_TimeWheel.h"
#include "GameSpecficAlgorithms/GameClient/Test_Client_Entity.h"

#include <signal.h>
#include <thread>


int UnitTesting()
{
    AlgorithmsUnitTesting gameAlgorighm;
    gameAlgorighm.TestServerPlayerManger();
    return 0;
}

int PerformanceTest()
{
    AOIPerformanceTest performance;
    performance.PerformanceTest();
    return 0;
}

int main()
{
    //std::cout << "start unit testing" << std::endl;
    //UnitTesting();
    //std::cout << "start PerformanceTest " << std::endl;
    //PerformanceTest();
    
    
   // Test_TimeWheel timeWheel;
    //timeWheel.TestAll();

    //TestClientEntity test;
    //test.Test();
    
    std::cout << "start game server " << std::endl;
    signal(SIGPIPE, SIG_IGN);
    
    auto& cfg = ConfigManager::getInstance();
    if (!cfg.loadConfig("./config/server.ini")) {
        std::cerr << "Failed to load config\n";
        return -1;
    }

    EventLoop loop;
   
    // tcpServer
    /*
    TcpServer server(&loop, PORT);
    server.SetMessageCallBack(std::bind(&TcpServer::HandleOnMessage, 
        &server, std::placeholders::_1, 
        std::placeholders::_2,
        std::placeholders::_3)
    );
    server.Start(0, 6);
    */


   

    // rpcServer
    int my_port = cfg.getInt("Rpc", "listen_port", 8888);
    //RpcServer server(&loop, my_port);
    //server.RegisterMethod("add", ProtoMethod::add);
    GameServer server(&loop, my_port);
    server.Start();

    // 不向服务注册中心注册了。
    /*
    std::string my_ip = cfg.getString("Rpc", "server_ip", "127.0.0.1");
    std::string registry_host = cfg.getString("RegisterCenter", "ip", "127.0.0.1");
    int registry_port = cfg.getInt("RegisterCenter", "port", 8888);
    int ttl_sec = cfg.getInt("RegisterCenter", "ttl_sec", 30);

    server.EnableServiceDiscovery(registry_host, registry_port, "rpc_server", my_ip, my_port, ttl_sec);
    */

    loop.Loop();
    return 0;
}


