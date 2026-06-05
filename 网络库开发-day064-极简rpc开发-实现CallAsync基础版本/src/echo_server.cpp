#include "EventLoopThread.h"
#include "EventLoopThreadPool.h"
#include "EventLoop.h"
#include "TcpServer.h"
#include "HttpServer.h"
#include "Common/JsonMethod.h"
#include "Common/ProtoMethod.h"
#include "LibRpc/RpcServer.h"
#include "LibRpc/RpcClient.h"
#include "LibRpc/RpcTestClientFile.h"
#include "LibRpc/RpcLogFile.h"
#include "TcpClient.h"
#include <iostream>
#include <chrono>
#include <iostream>

#include <signal.h>
#include <thread>


#define PORT 8888


int main()
{
    
    signal(SIGPIPE, SIG_IGN);
    
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
    server.Start(0, 6, 5);
    */


    // httpSrver
    /*
    HttpServer server(&loop, PORT);
    server.RegisterMethod("add", JsonMethodLib::add);
    server.Start(0, 6, 0); // eventloopThread 6个工作线程为最佳性能
    */

    // rpcServer
    RpcServer server(&loop, PORT);
    server.RegisterMethod("add", ProtoMethod::add);
    //server.RegisterMethod("echo", JsonMethodLib::echo);
    //server.RegisterMethod("login", JsonMethodLib::login);
    server.Start(0, 6, 0);

    
    //EventLoop* pLoop = server.GetIndexLoop(5);
    //pLoop->RunAfter(std::chrono::seconds(5), client_function);

    loop.Loop();
    rpcLog.Release();
    return 0;
}


int ClientPressTest(int threads, int req_per_threads)
{
    //const int threads = 10;
    //const int req_per_threads = 10000; // 每个线程执行的次数
    std::vector<std::thread> thread_pool;
    std::vector<std::vector<uint64_t>> all_latencies(threads);

    auto overall_start = std::chrono::steady_clock::now();
    for(int i = 0; i < threads; ++i)
    {
        thread_pool.emplace_back(client_work_function, i, req_per_threads, std::ref(all_latencies[i]));
    }


    for(auto& th : thread_pool)
    {
        th.join();
    }

    auto overall_end = std::chrono::steady_clock::now();
    double total_sec = std::chrono::duration<double>(overall_end - overall_start).count();
    size_t total_req = threads * req_per_threads;
    double total_qps = total_req / total_sec;

    // 合并所有延迟
    std::vector<uint64_t> all_us;
    for(auto& latencies : all_latencies)
    {
        all_us.insert(all_us.end(), latencies.begin(), latencies.end());
    }

    double avg_us = std::accumulate(all_us.begin(), all_us.end(), 0.0) / all_us.size();

    std::sort(all_us.begin(), all_us.end());
    auto precentile_us_func = [&](double percent) -> uint64_t {
        if(all_us.empty())
        {
            return 0;
        }

        size_t idx = percent * (all_us.size() - 1);
        if(idx >= all_us.size())
        {
            idx = all_us.size() - 1;
        }

        return all_us[idx];
    };

    std::cout << "all_us vector size(): " << all_us.size() << std::endl;
    std::cout << "Total request: " << total_req << std::endl;
    std::cout << "Total time(second): " << total_sec << std::endl;
    std::cout << "QPS: " << total_qps << std::endl;
    std::cout << "Average latency: " << avg_us << std::endl;

    std::cout << "P50 latency: " << precentile_us_func(0.50) << std::endl;
    std::cout << "P90 latency: " << precentile_us_func(0.90) << std::endl;
    std::cout << "P99 latency: " << precentile_us_func(0.99) << std::endl;
    std::cout << "P999 latency: " << precentile_us_func(0.999) << std::endl;

    return 0;
}


int test_client_function()
{
    RpcLogFile& rpcLog = RpcLogFile::getInstance();
    rpcLog.OpenFile("rpc_client_log.txt");

    std::cout << "start 50 thread and req_per_threads 10000: " << std::endl;
    ClientPressTest(50, 10000);
    std::cout << std::endl << std::endl;

    std::cout << "start 20 thread and req_per_threads 10000: " << std::endl;
    ClientPressTest(20, 10000);
    std::cout << std::endl << std::endl;

    
    std::cout << "start 10 thread and req_per_threads 10000: " << std::endl;
    ClientPressTest(10, 10000);
    std::cout << std::endl << std::endl;
    
     
    std::cout << "start 5 thread and req_per_threads 10000: " << std::endl;
    ClientPressTest(5, 10000);
    std::cout << std::endl << std::endl;
    

    
    std::cout << "start 1 thread and req_per_threads 10000: " << std::endl;
    ClientPressTest(1, 10000);
    std::cout << std::endl << std::endl;
    

    rpcLog.Release();
    return 0;
}