#pragma once

#include "../EventLoop.h"
#include "RpcClient.h"
#include "RpcErrorCodeDef.h"
#include "../TcpClient.h"
#include "../TcpConnection.h"
#include "../Decoder/LengthPrefixDecoder.h"
#include "../Common/JsonMethod.h"
#include "../../build/proto_gen/add.pb.h"
#include "RpcLogFile.h"

#include <iostream>
#include <chrono>
#include <iostream>

#include <signal.h>
#include <thread>
#include <atomic>
#include <chrono>
#include <numeric>
#include <algorithm>
//#include <nlohmann/json.hpp>




void client_work_function(int id, int task_count, std::vector<uint64_t>& latencies, std::atomic<uint64_t>& timeout_cnt) {
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
            auto length_decoder = std::make_unique<LengthPrefixDecoder>();
            conn->SetDecoder(std::move(length_decoder));
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

    std::atomic<size_t> pending{0};
    latencies.reserve(task_count);

    int add_num = id * task_count;
    AddRequest req;
    req.set_a(add_num);
    req.set_b(0);

    pending = task_count;
    for(int i = 0; i < task_count; ++i)
    {

       try {
            
            auto overall_start = std::chrono::steady_clock::now();
            req.set_b(i);   
            int expected_sum = add_num + i;

            // 暂时不使用同步回调，使用异步回调进行测试
            /*
            std::string strResult = rpcClient->Call("add", req.SerializeAsString(), 5000);
            AddResponse response;
            response.ParseFromString(strResult);

            //std::cout << "RPC result: " << response.sum() << std::endl;
            assert(expected_sum == response.sum());
            auto overall_end = std::chrono::steady_clock::now();
            auto cost_sec = std::chrono::duration_cast<std::chrono::microseconds>(overall_end - overall_start).count();
            latencies.emplace_back(cost_sec);
            */

            
            rpcClient->CallAsync("add", req.SerializeAsString(), [&pending, &loop, expected_sum, overall_start, &latencies, &timeout_cnt](const std::string& strResult, int32_t error){
                AddResponse response;
                response.ParseFromString(strResult);

                if(0 == error)
                {
                    //std::cout << "RPC result: " << response.sum() << std::endl;
                    assert(expected_sum == response.sum());
                    auto overall_end = std::chrono::steady_clock::now();
                    auto cost_sec = std::chrono::duration_cast<std::chrono::microseconds>(overall_end - overall_start).count();

                    latencies.emplace_back(cost_sec);
                }else{
                    std::cout << "RPC error: " << error << std::endl;
                    if(eRpcCode_TimeOut == error)
                    {
                        timeout_cnt.fetch_add(1, std::memory_order_relaxed);
                    }
                }

                // 异步的时候添加代码
                --pending;
                if(0 == pending)
                {
                    // 停止 I/O 线程（可选，简单退出）
                    loop.Quit();
                }
            });

            
        } catch (const std::exception& e) {
            std::cerr << "RPC error: " << e.what() << std::endl;
            
            --pending;
            if(0 == pending)
            {
                // 停止 I/O 线程（可选，简单退出）
                loop.Quit();
            }
        }       
    }
    

    // 停止 I/O 线程（可选，简单退出）
    //loop.Quit(); // 同步的时候终止代码,异步的时候屏蔽该代码
    io_thread.join();
    //return 0;
}






