
#include <thread>
#include <chrono>
#include "LibRpc/RpcConnectionPool.h"
#include "LibRpc/RpcClient.h"
#include "../build/proto_gen/add.pb.h"
#include "LibRpc/RpcErrorCodeDef.h"
#include "Common/ProtoMethod.h"
#include "Common/ConfigManager.h"
#include "Common/ObjectPool.h"
#include "EventLoop.h"


int ObjectPool_test()
{
    struct Test {
        int a, b;
        Test(int x, int y) : a(x), b(y) { std::cout << "Construct " << a << "," << b << std::endl; }
        ~Test() { std::cout << "Destruct " << a << "," << b << std::endl; }
    };

    ObjectPool<Test> pool(100);
    //Test* t1 = pool.Allocate(1,3);
    return 0;
}

int echo_rpc_client_test() 
{
    auto& cfg = ConfigManager::getInstance();
    if (!cfg.loadConfig("./config/client.ini")) {
        std::cerr << "Failed to load config\n";
        return -1;
    }

    std::string strIp = cfg.getString("RegisterCenter", "ip", "127.0.0.1");
    int port = cfg.getInt("RegisterCenter", "port", 8888);

    RpcConnectionPool pool;                     // 空连接池
    pool.EnableAutoDiscovery(strIp, port, "rpc_server", 30);
    
    // 等待第一次拉取完成（或者 Sleep 几秒）
    std::this_thread::sleep_for(std::chrono::seconds(5));
    
    auto rpcClient = pool.GetConnection();         // 此时已有连接
    if(!rpcClient)
    {
        std::cerr << "pool get connection error" << std::endl;
        return -1;
    }
    
    EventLoop loop;
    std::thread io_thread([&loop](){
        loop.Loop();
    });

    AddRequest req;
    req.set_a(1);
    req.set_b(0);
    for(int i = 0; i < 10; ++i)
    {
        req.set_b(i);
        int expected_sum = 1 + i;

        rpcClient->CallAsync("add", req.SerializeAsString(), [expected_sum, &loop, i](const std::string& result, int32_t error){

            AddResponse response;
            response.ParseFromString(result);

            std::cout << "CallAsync RPC result: " << response.sum() << " Expect sum:=" << expected_sum << std::endl;
            //assert(expected_sum == response.sum());

            if((i+1) >= 10)
            {
                loop.Quit();
            }

        }, 5000);
    }
    
    // 不能让他马上结束
    io_thread.join();
    return 0;
}