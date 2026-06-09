
#include "EventLoopThread.h"
#include "EventLoopThreadPool.h"
#include "EventLoop.h"
#include "TcpServer.h"
#include "Http/HttpServer.h"
#include "Common/ProtoMethod.h"
#include "Common/ConfigManager.h"
#include "LibRpc/RpcServer.h"
#include "LibRpc/RpcClient.h"
#include "LibRpc/RpcTestClientFile.h"
#include "LibRpc/RpcLogFile.h"
#include "LibRpc/RpcConnectionPool.h"
#include "Http/SimpleHttpClient.h"
#include "TcpClient.h"
#include <iostream>
#include <chrono>
#include <iostream>

#include <signal.h>
#include <thread>



int ClientPressTest(int threads, int req_per_threads)
{
    //const int threads = 10;
    //const int req_per_threads = 10000; // 每个线程执行的次数
    std::vector<std::thread> thread_pool;
    std::vector<std::vector<uint64_t>> all_latencies(threads);


    RpcConnectionPool connection_pool;
    std::atomic<uint64_t> timeout_cnt;

    uint64_t total_tasks = threads * req_per_threads;
    std::atomic<uint64_t> pending_count{total_tasks};
    
    std::condition_variable cv;

    auto overall_start = std::chrono::steady_clock::now();
    for(int i = 0; i < threads; ++i)
    {
        thread_pool.emplace_back(
            client_work_function, 
            i, 
            req_per_threads, 
            std::ref(all_latencies[i]), 
            std::ref(timeout_cnt), 
            std::ref(connection_pool),
            std::ref(cv),
            std::ref(pending_count)
        );
    }


    for(auto& th : thread_pool)
    {
        th.join();
    }

    {
        std::mutex mutex;
        std::unique_lock<std::mutex> lk(mutex);
        cv.wait(lk, [&pending_count] () {
            return 0 == pending_count.load();
        });
    }

    auto overall_end = std::chrono::steady_clock::now();
    double total_sec = std::chrono::duration<double>(overall_end - overall_start).count();
    size_t total_req = threads * req_per_threads;
    
   
    // 合并所有延迟
    std::vector<uint64_t> all_us;
    for(auto& latencies : all_latencies)
    {
        all_us.insert(all_us.end(), latencies.begin(), latencies.end());
    }

    double total_qps = all_us.size() / total_sec;
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

    
    std::cout << "Total request: " << total_req << std::endl;
    std::cout << "Success: " << all_us.size();
    std::cout << " , Timeout: " << timeout_cnt.load(std::memory_order_relaxed);
    std::cout << " , Fail:" << (total_req - all_us.size() - timeout_cnt)  << std::endl;

    std::cout << "Total time(second): " << total_sec << std::endl;
    std::cout << "QPS: " << total_qps << std::endl;
    std::cout << "Average latency: " << avg_us << std::endl;

    std::cout << "P50 latency: " << precentile_us_func(0.50) << std::endl;
    std::cout << "P90 latency: " << precentile_us_func(0.90) << std::endl;
    std::cout << "P99 latency: " << precentile_us_func(0.99) << std::endl;
    std::cout << "P999 latency: " << precentile_us_func(0.999) << std::endl;

    
    return 0;
}


int test_http_client()
{
    auto& cfg = ConfigManager::getInstance();
    std::string strIp = cfg.getString("Network", "ip", "127.0.0.1");
    int port = cfg.getInt("Network", "port", 8888);

    std::string path = "/add";
    std::string body = R"({"a":10,"b":32})";
    
    for(int i = 0 ; i < 10; ++i)
    {
         SimpleHttpClient::Response response = SimpleHttpClient::Post(strIp, port, path, body);

         response = SimpleHttpClient::Get(strIp, port, path);
    }
   
   
/*
   response = SimpleHttpClient::Post(strIp, port, path, body);
    {
         std::cout << "SimpleHttpClient::Response_222  success:" <<  response.success;
         std::cout << " status_code:" <<  response.status_code;
         std::cout << " body:" <<  response.body;
         std::cout << " error_msg:" <<  response.error_msg;
         std::cout << std::endl;
    }
         */

    return 0;
}

int main()
{
    auto& cfg = ConfigManager::getInstance();
    if (!cfg.loadConfig("./config/client.ini")) {
        std::cerr << "Failed to load config\n";
        return -1;
    }

    RpcLogFile& rpcLog = RpcLogFile::getInstance();
    rpcLog.OpenFile("rpc_client_log.txt");

    test_http_client();

    /*
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
    */
   
    rpcLog.Release();
    return 0;
}