
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
#include "GameSpecficAlgorithms/GameTest/AOIPerformanceTest.h"
#include "GameSpecficAlgorithms/GameTest/Test_TimeWheel.h"
#include "GameSpecficAlgorithms/GameClient/Test_Client_Entity.h"
#include "GameSpecficAlgorithms/GameTest/BehaviorTreeAction_TestFile.h"
#include "GameSpecficAlgorithms/GameTest/Test_Migration.h"
#include "GameSpecficAlgorithms/GameTest/Test_A_Star.h"
#include "GameSpecficAlgorithms/GameTest/Test_Move_To_Target.h"
#include "GameSpecficAlgorithms/GameTest/Test_NavMesh.h"
#include "GameSpecficAlgorithms/GameTest/Test_Rvo2_Agent.h"
#include "GameSpecficAlgorithms/GameTest/Test_DB_Mysql.h"

#include <signal.h>
#include <thread>
#include <mysql/mysql.h>


int UnitTesting()
{
    AlgorithmsUnitTesting gameAlgorighm;
    //gameAlgorighm.TestDynamicAOI_V2();
    //gameAlgorighm.TestPartitionCreation();
    gameAlgorighm.Test_IncrementalHashTable();
    return 0;
}

int PerformanceTest()
{
    AOIPerformanceTest performance;
    performance.PerformanceTest();
    return 0;
}

int MysqlTest()
{
    std::cout << "MySQL client version: " << mysql_get_client_info() << std::endl;
    
    // 测试连接对象（只是验证 API 能调用）
    MYSQL* conn = mysql_init(nullptr);
    if (conn) {
        std::cout << "mysql_init() succeeded" << std::endl;
        mysql_close(conn);
    } else {
        std::cout << "mysql_init() failed" << std::endl;
        return 1;
    }

    return 0;
}


int main()
{
    //std::cout << "start unit testing" << std::endl;
    //UnitTesting();
    //MysqlTest();
    Test_DB_Mysql mysql_test;
    mysql_test.Test_Connection_Pool();
    
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


