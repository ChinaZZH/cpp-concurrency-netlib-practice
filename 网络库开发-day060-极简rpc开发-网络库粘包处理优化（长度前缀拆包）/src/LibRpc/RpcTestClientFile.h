#include "../EventLoop.h"
#include "RpcClient.h"
#include "../TcpClient.h"
#include "../TcpConnection.h"

#include <iostream>
#include <chrono>
#include <iostream>

#include <signal.h>
#include <thread>
#include <nlohmann/json.hpp>

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


struct LoginRequest{
    std::string user;
    std::string pass;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LoginRequest, user, pass);


struct LoginResponse{
    int code;
    std::string token;
    std::string msg;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(LoginResponse, code, token, msg);


void client_work_function(int id, int task_count, std::vector<uint64_t>& latencies) {
    // 创建 EventLoop 对象（将在独立线程中运行）
    EventLoop loop;

    // 创建 TcpClient 和 RpcClient
    auto client = std::make_shared<TcpClient>(&loop, "127.0.0.1", 8888);
    auto rpcClient = std::make_shared<RpcClient>(nullptr);

    // 用于等待连接建立的同步
    std::mutex mtx;
    std::condition_variable cv;
    bool connected = false;

    std::weak_ptr<RpcClient> rpcPtr = rpcClient->shared_from_this();
    client->SetConnectionCallBack([&, rpcPtr](const TcpConnectionPtr& conn) {
        auto rpcClient = rpcPtr.lock();
        if(rpcClient)
        {
            //std::cout << "Connected to server" << std::endl;
            rpcClient->SetConnection(conn);
            {
                std::lock_guard<std::mutex> lock(mtx);
                connected = true;
            }
        }
        

        cv.notify_one();
    });
    
    client->SetMessageCallBack([rpcPtr](const TcpConnectionPtr&, std::string& msg) {
        auto rpcClient = rpcPtr.lock();
        if(rpcClient)
        {
             rpcClient->OnResponse(msg);
        }
    });

    client->Connect();

    // 启动 I/O 线程
    std::thread io_thread([&loop]() {
        //std::cout << "io_thread thread_id:=" << std::this_thread::get_id() << std::endl;
        loop.Loop();
    });
    
    // 等待连接建立
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&] { return connected; });
    }

    // 主线程发起 add RPC 调用
    /*
    try {
        std::string result = rpcClient->Call("add", R"({"a":10,"b":32})", 1000);
        std::cout << "RPC result: " << result << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "RPC error: " << e.what() << std::endl;
    }

    // 主线程发起 add RPC 调用
    try {
        AddRequest req{100, 320};
        AddResponse response = rpcClient->Call<AddRequest, AddResponse>("add", req, 1000);
        std::cout << "RPC result: " << response.result << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "RPC error: " << e.what() << std::endl;
    }

    // 主线程发起 echo RPC 调用
    try {
        std::string result = rpcClient->Call("echo", "hello kitty", 1000);
        std::cout << "RPC result: " << result << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "RPC error: " << e.what() << std::endl;
    }

    // 主线程发起 login RPC 调用
    try {
        std::string result = rpcClient->Call("login", R"({"user":"admin","pass":"123"})", 1000);
        std::cout << "RPC result: " << result << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "RPC error: " << e.what() << std::endl;
    }

    // 主线程发起 login RPC 调用
    try {
        LoginRequest req{"admin", "abc123"};
        LoginResponse response = rpcClient->Call<LoginRequest, LoginResponse>("login", req, 1000);
        std::cout << "RPC reuslt code:= " << response.code  << " token:=" << response.token << " msg:=" << response.msg << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "RPC error: " << e.what() << std::endl;
    }
    */

    latencies.reserve(task_count);

    int add_num = id * task_count;
    AddRequest req{add_num, 0};
    for(int i = 0; i < task_count; ++i)
    {

       auto overall_start = std::chrono::steady_clock::now();
       try {
            req.b = i;    
            AddResponse response = rpcClient->Call<AddRequest, AddResponse>("add", req, 5000);
            //std::cout << "RPC result: " << response.result << std::endl;
            assert(add_num + i == response.result);
            auto overall_end = std::chrono::steady_clock::now();
            auto cost_sec = std::chrono::duration_cast<std::chrono::microseconds>(overall_end - overall_start).count();
            latencies.emplace_back(cost_sec);
        } catch (const std::exception& e) {
            std::cerr << "RPC error: " << e.what() << std::endl;
        }

        //std::this_thread::sleep_for(std::chrono::microseconds(200));
    }
    

    // 停止 I/O 线程（可选，简单退出）
    loop.Quit();
    io_thread.join();
    //return 0;
}




std::string add(const std::string& params)
{
    //std::this_thread::sleep_for(std::chrono::seconds(2));
    int result = 0;
    try
    {
        auto j = json::parse(params);
        int a =  j["a"];
        int b = j["b"];
        result = a + b;
    }
    catch(const std::exception& e)
    {
        throw std::runtime_error("call add function error");
    }
    
    json res = {{"result", result}};
    return res.dump();
}


std::string echo(const std::string& params)
{
    return params;
}

std::string login(const std::string& params)
{
    try
    {
        auto j = json::parse(params);
        if("admin" == j["user"] && "123" == j["pass"])
        {
            return R"({"code":0,"token":"abc123", "msg":"auth successed"})";
        }else{
            return R"({"code":1,"token":"", "msg":"auth failed"})";
        }
    }
    catch(const std::exception& e)
    {
        throw std::runtime_error("call login function error");
    }

    return R"({"code":1,"token":"", "msg":"auth failed"})";
}