
#include <thread>
#include <chrono>
#include "LibRpc/RpcConnectionPool.h"
#include "LibRpc/RpcClient.h"
#include "../build/proto_gen/add.pb.h"
#include "../build/proto_gen/aoi.pb.h"
#include "LibRpc/RpcErrorCodeDef.h"
#include "Common/ProtoMethod.h"
#include "Common/ConfigManager.h"
#include "Common/ObjectPool.h"
#include "EventLoop.h"
#include "Decoder/LengthAndTypePrefixDecoder.h"
#include "GameSpecficAlgorithms/GameServerMsgTypeDefine.h"


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

    /*
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
    */

    aoi::EntityEnterRequest newRequest;
    newRequest.set_map_id(100);
    aoi::EntityInfo* pEntity = newRequest.mutable_new_entity();

    pEntity->set_entity_id(1);
    pEntity->set_x(50);
    pEntity->set_y(50);
    std::string strContent = std::move(LengthAndTypePrefixDecoder::MakeRequestString(newRequest.SerializeAsString(), GSMT_AddEntity));
    rpcClient->CallAsyncIgnoreResponse(strContent);

   
    //aoi.AddEntity(2, 120, 50);
    pEntity->set_entity_id(2);
    pEntity->set_x(120);
    pEntity->set_y(50);
    strContent = std::move(LengthAndTypePrefixDecoder::MakeRequestString(newRequest.SerializeAsString(), GSMT_AddEntity));
    rpcClient->CallAsyncIgnoreResponse(strContent);


    pEntity->set_entity_id(3);
    pEntity->set_x(50);
    pEntity->set_y(120);
    strContent = std::move(LengthAndTypePrefixDecoder::MakeRequestString(newRequest.SerializeAsString(), GSMT_AddEntity));
    rpcClient->CallAsyncIgnoreResponse(strContent);
  
    pEntity->set_entity_id(4);
    pEntity->set_x(200);
    pEntity->set_y(200);
    strContent = std::move(LengthAndTypePrefixDecoder::MakeRequestString(newRequest.SerializeAsString(), GSMT_AddEntity));
    rpcClient->CallAsyncIgnoreResponse(strContent);

    // 移动实体
    aoi::EntityMoveRequest moveReq;
    moveReq.set_map_id(100);
    moveReq.set_entity_id(2);
    moveReq.set_new_x(100);
    moveReq.set_new_y(50);
    strContent = std::move(LengthAndTypePrefixDecoder::MakeRequestString(moveReq.SerializeAsString(), GSMT_MoveEntity));
    rpcClient->CallAsyncIgnoreResponse(strContent);
    
    moveReq.set_new_x(300);
    moveReq.set_new_y(300);
    strContent = std::move(LengthAndTypePrefixDecoder::MakeRequestString(moveReq.SerializeAsString(), GSMT_MoveEntity));
    rpcClient->CallAsyncIgnoreResponse(strContent);

    // 删除实体
    aoi::EntityLeaveRequest removeReq;
    removeReq.set_map_id(100);
    removeReq.set_entity_id(3);
    strContent = std::move(LengthAndTypePrefixDecoder::MakeRequestString(removeReq.SerializeAsString(), GSMT_RemoveEntity));
    rpcClient->CallAsyncIgnoreResponse(strContent);

    std::this_thread::sleep_for(std::chrono::seconds(60));
    // 不能让他马上结束
    io_thread.join();
    return 0;
}