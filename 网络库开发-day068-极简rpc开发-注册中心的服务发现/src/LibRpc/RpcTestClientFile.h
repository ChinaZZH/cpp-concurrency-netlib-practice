#pragma once

#include "../EventLoop.h"
#include "RpcClient.h"
#include "RpcErrorCodeDef.h"
#include "../TcpClient.h"
#include "../TcpConnection.h"
#include "../Decoder/LengthPrefixDecoder.h"
#include "../Common/JsonMethod.h"
#include "../LibRpc/RpcConnectionPool.h"
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
#include <condition_variable>
#include <mutex>
//#include <nlohmann/json.hpp>




void client_work_function(int id, int task_count, std::vector<uint64_t>& latencies, 
    std::atomic<uint64_t>& timeout_cnt, RpcConnectionPool& connection_pool, 
    std::condition_variable& cv, std::atomic<uint64_t>& total_pendings) 
{

    std::atomic<size_t> pending{0};
    latencies.reserve(task_count);

    int add_num = id * task_count;
    AddRequest req;
    req.set_a(add_num);
    req.set_b(0);

    pending = task_count;
    RpcConnectionPool::RpcClientPtr rpcClient = connection_pool.GetConnection();
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


            
            rpcClient->CallAsync("add", req.SerializeAsString(), [&total_pendings, &pending, &rpcClient, expected_sum, overall_start, &latencies, &timeout_cnt, &cv](const std::string& strResult, int32_t error){
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
                if(1 == total_pendings.fetch_sub(1))
                {
                    // 停止 I/O 线程（可选，简单退出）
                    cv.notify_one();
                }
            });

            
        } catch (const std::exception& e) {
            std::cerr << "RPC error: " << e.what() << std::endl;
            

            if(0 == total_pendings.fetch_sub(1))
            {
                // 停止 I/O 线程（可选，简单退出）
                cv.notify_one();
            }
        }       
    }
    

    // 停止 I/O 线程（可选，简单退出）
    //loop.Quit(); // 同步的时候终止代码,异步的时候屏蔽该代码
    
    //return 0;
}






