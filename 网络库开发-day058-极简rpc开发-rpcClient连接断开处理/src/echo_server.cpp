#include "EventLoopThread.h"
#include "EventLoopThreadPool.h"
#include "EventLoop.h"
#include "TcpServer.h"
#include "HttpServer.h"
#include "LibRpc/RpcServer.h"
#include "LibRpc/RpcClient.h"
#include "TcpClient.h"
#include <iostream>
#include <chrono>
#include <iostream>

#include <signal.h>
#include <thread>
#include <nlohmann/json.hpp>

#define PORT 8888

using json = nlohmann::json;

struct AddRequest{
    int a;
    int b;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AddRequest, a, b);

/*
void to_json(nlohmann::json& j, const AddRequest& req)
{
    j = nlohmann::json{{"a", req.a} , {"b", req.b}};
}
*/

struct AddResponse{
    int result;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(AddResponse, result);

/*
void from_json(const nlohmann::json& j, AddResponse& resp)
{
    j.at("result").get_to(resp.result);
}
*/

void client_function() {
    // 创建 EventLoop 对象（将在独立线程中运行）
    EventLoop loop;

    // 创建 TcpClient 和 RpcClient
    auto client = std::make_shared<TcpClient>(&loop, "127.0.0.1", 8888);
    auto rpcClient = std::make_shared<RpcClient>(nullptr);

    // 用于等待连接建立的同步
    std::mutex mtx;
    std::condition_variable cv;
    bool connected = false;

    client->SetConnectionCallBack([&, rpcClient](const TcpConnectionPtr& conn) {
        std::cout << "Connected to server" << std::endl;
        rpcClient->SetConnection(conn);
        {
            std::lock_guard<std::mutex> lock(mtx);
            connected = true;
        }
        cv.notify_one();
    });
    
    client->SetMessageCallBack([rpcClient](const TcpConnectionPtr&, std::string& msg) {
        rpcClient->OnResponse(msg);
    });

    client->Connect();

    // 启动 I/O 线程
    std::thread io_thread([&loop]() {
        loop.Loop();
    });
    
    // 等待连接建立
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&] { return connected; });
    }

    // 主线程发起 RPC 调用
    try {
        std::string result = rpcClient->Call("add", R"({"a":10,"b":32})", 1000);
        std::cout << "RPC result: " << result << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "RPC error: " << e.what() << std::endl;
    }

    // 主线程发起 RPC 调用
    try {
        AddRequest req{100, 320};
        AddResponse response = rpcClient->Call<AddRequest, AddResponse>("add", req, 1000);
        std::cout << "RPC result: " << response.result << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "RPC error: " << e.what() << std::endl;
    }

    // 停止 I/O 线程（可选，简单退出）
    loop.Quit();
    io_thread.join();
    //return 0;
}



std::string add(const std::string& params)
{
    //std::this_thread::sleep_for(std::chrono::seconds(2));
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
    EventLoop* pLoop = server.GetIndexLoop(5);
    pLoop->RunAfter(std::chrono::seconds(5), client_function);

    loop.Loop();
    return 0;
}


