
#include <thread>
#include <chrono>
#include "LibRpc/RpcConnectionPool.h"
#include "LibRpc/RpcClient.h"
#include "../build/proto_gen/add.pb.h"
#include "LibRpc/RpcErrorCodeDef.h"
#include "Common/ProtoMethod.h"
#include "Common/ConfigManager.h"

int main() 
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
    
    AddRequest req;
    req.set_a(1);
    req.set_b(0);
    for(int i = 0; i < 10; ++i)
    {
        req.set_b(i);
        int expected_sum = 1 + i;

        std::string strResult = rpcClient->Call("add", req.SerializeAsString(), 5000);
        AddResponse response;
        response.ParseFromString(strResult);

        std::cout << "RPC result: " << response.sum() << " Expect sum:=" << expected_sum << std::endl;
        //assert(expected_sum == response.sum());
    }
    

    return 0;
}