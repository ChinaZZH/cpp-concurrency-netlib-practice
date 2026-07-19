#pragma once

#include "../EventLoop.h"
#include "RpcClient.h"
#include "RpcErrorCodeDef.h"
#include "../TcpClient.h"
#include "../TcpConnection.h"
#include "../Decoder/LengthPrefixDecoder.h"
#include "../Decoder/LengthAndTypePrefixDecoder.h"
#include "../Common/JsonMethod.h"
#include "../LibRpc/RpcConnectionPool.h"
#include "../../build/proto_gen/add.pb.h"
#include "../../build/proto_gen/aoi.pb.h"
#include "../../build/proto_gen/frame_sync.pb.h"
#include "../GameSpecficAlgorithms/FrameSync/ClientInputSender.h"
#include "../GameSpecficAlgorithms/GameServerMsgTypeDefine.h"
#include "../GameSpecficAlgorithms/GameClient/ClientEntityMgr.h"
#include "../GameSpecficAlgorithms/FrameSync/ClientPredictionManager.h"

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
#include <coroutine>




void connection_pool_client_work_function(int id, int task_count, std::vector<uint64_t>& latencies, 
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



void client_work_function(bool async_call, int id, int task_count, std::vector<uint64_t>& latencies, std::atomic<uint64_t>& timeout_cnt) {
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
    
    client->SetMessageCallBack([rpcPtr](const TcpConnectionPtr&, std::string& msg, uint32_t msgType) {
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
            if(false == async_call)
            {
                std::string strResult = rpcClient->Call("add", req.SerializeAsString(), 5000);
                AddResponse response;
                response.ParseFromString(strResult);

                //std::cout << "RPC result: " << response.sum() << std::endl;
                assert(expected_sum == response.sum());
                auto overall_end = std::chrono::steady_clock::now();
                auto cost_sec = std::chrono::duration_cast<std::chrono::microseconds>(overall_end - overall_start).count();
                latencies.emplace_back(cost_sec);
            }else{
                rpcClient->CallAsync("add", req.SerializeAsString(), [&pending, &loop, expected_sum, overall_start, &latencies, &timeout_cnt](const std::string& strResult, int32_t error){
                    AddResponse response;
                    response.ParseFromString(strResult);

                    if(0 == error){
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
            }
        
        } catch (const std::exception& e) {
            std::cerr << "RPC error: " << e.what() << std::endl;
            if(async_call)
            {
                --pending;
                if(0 == pending)
                {
                    // 停止 I/O 线程（可选，简单退出）
                    loop.Quit();
                }
            }
           
        }       
    }
    

    // 停止 I/O 线程（可选，简单退出）
    if(false == async_call)
    {
        loop.Quit(); // 同步的时候终止代码,异步的时候屏蔽该代码
    }
    
    io_thread.join();
    //return 0;
}




void test_game_server_aoi_function(bool async_call, int id, int task_count, std::vector<uint64_t>& latencies, std::atomic<uint64_t>& timeout_cnt) {
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
            auto msg_type_decoder = std::make_unique<LengthAndTypePrefixDecoder>();
            conn->SetDecoder(std::move(msg_type_decoder));
            rpcClient->SetConnection(conn);
            {
                std::lock_guard<std::mutex> lock(mtx);
                connected = true;
            }
        }
        

        cv.notify_one();
    });
    

    ClientEntityMgr clientEntity(1); // 定义客户端为entityId为1, 这边只弄entityId为1的客户端，只看他的视野。
    clientEntity.Start(); // 启动客户端实体管理器的更新循环
    client->SetMessageCallBack([rpcPtr, &clientEntity](const TcpConnectionPtr&, std::string& msg, uint32_t msgType) {
        std::cout << "onMessageCallBack:=" << msgType << std::endl;
        switch(msgType)
        {
            case GSMT_AddEntity:
            {
                aoi::EntityEnterNotifyResponse response;
                if(!response.ParseFromString(msg))
                {
                    std::cout << "client onMessageCallBack parse GSMT_AddEntity error!!!" << std::endl;
                }
                else{
                   
                    // std::cout << " position x:=" << response.new_entity().x() << " position y:=" << response.new_entity().y() << std::endl;
                    if(clientEntity.GetCurrentEntityId() == response.msg_client_id())
                    {
                         std::cout << "new entity id:=" << response.msg_client_id();
                        clientEntity.AddNewEntity(response.new_entity().entity_id(), response.new_entity().x(), response.new_entity().y());
                    }
                }
            }
            break;
                
            case GSMT_RemoveEntity:
            {
                aoi::EntityLeaveNotifyResponse response;
                if(!response.ParseFromString(msg))
                {
                    std::cout << "client onMessageCallBack parse GSMT_RemoveEntity error!!!" << std::endl;
                }
                else{
                    
                    if(clientEntity.GetCurrentEntityId() == response.msg_client_id())
                    {
                        std::cout << "remove entity id:=" << response.msg_client_id();
                        clientEntity.RemoveEntity(response.entity_id());
                    }
                }
            }
            break;

            case GSMT_MoveEntity:
             {
                aoi::EntityMoveNotifyResponse response;
                if(!response.ParseFromString(msg))
                {
                    std::cout << "client onMessageCallBack parse GSMT_MoveEntity error!!!" << std::endl;
                }
                else{
                    
                     // std::cout << "move entity id:=" << response.entity_id();
                    // std::cout << " position x:=" << response.new_x() << " position y:=" << response.new_y() << std::endl;
                    if(clientEntity.GetCurrentEntityId() == response.msg_client_id())
                    {
                        std::cout << "move entity id:=" << response.msg_client_id();
                        clientEntity.MoveEntity(response.entity_id(), response.new_x(), response.new_y());
                    }
                }
            }
            break;

            case GSMT_SyncNeighborsEntity:
            {
                aoi::InitAroundEntitiesNotifyResponse response;
                if(!response.ParseFromString(msg))
                {
                    std::cout << "client onMessageCallBack parse GSMT_SyncNeighborsEntity error!!!" << std::endl;
                }
                else{
                    // 只打印特定玩家的视野
                    
                    if(clientEntity.GetCurrentEntityId() == response.msg_client_id())
                    {
                        std::cout << "around msg entity id:=" << response.msg_client_id();
                        for(int i = 0; i < response.around_entities_size(); i = i + 1)
                        {
                            const aoi::EntityInfo& info = response.around_entities(i);
                            clientEntity.AddNewEntity(info.entity_id(), info.x(), info.y());
                            // std::cout << i << ":Sync index entity id:=" << response.around_entities(i).entity_id();
                            // std::cout << " position x:=" << response.around_entities(i).x() << " position y:=" << response.around_entities(i).y() << std::endl;
                        }
                    }
                    
                }
            }
            break;
            default:
                break;
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



    std::atomic<size_t> pending{0};
    latencies.reserve(task_count);

    int add_num = id * task_count;
    AddRequest req;
    req.set_a(add_num);
    req.set_b(0);

    pending = task_count;
    
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
    
    moveReq.set_new_x(120);
    moveReq.set_new_y(120);
    strContent = std::move(LengthAndTypePrefixDecoder::MakeRequestString(moveReq.SerializeAsString(), GSMT_MoveEntity));
    rpcClient->CallAsyncIgnoreResponse(strContent);

    // 删除实体
    aoi::EntityLeaveRequest removeReq;
    removeReq.set_map_id(100);
    removeReq.set_entity_id(3);
    strContent = std::move(LengthAndTypePrefixDecoder::MakeRequestString(removeReq.SerializeAsString(), GSMT_RemoveEntity));
    rpcClient->CallAsyncIgnoreResponse(strContent);


    std::this_thread::sleep_for(std::chrono::seconds(60));
    std::cout << "end !!!!" << std::endl;
    loop.Quit();
    io_thread.join();
    //return 0;
}




void test_game_server_frame_sync() {
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
            auto msg_type_decoder = std::make_unique<LengthAndTypePrefixDecoder>();
            conn->SetDecoder(std::move(msg_type_decoder));
            rpcClient->SetConnection(conn);
            
            {
                std::lock_guard<std::mutex> lock(mtx);
                connected = true;
            }
        }
        

        cv.notify_one();
    });
    

    int defaultPlayerID = 1;
    ClientPredictionManager clientFrame(defaultPlayerID, nullptr);
    client->SetMessageCallBack([rpcPtr, defaultPlayerID, &clientFrame](const TcpConnectionPtr&, std::string& msg, uint32_t msgType) {
        switch(msgType)
        {
            case GSMT_FrameSyncAckPackage:
            {
                TestAckPackage ack;
                if(!ack.ParseFromString(msg))
                {
                    std::cout << "client onMessageCallBack parse GSMT_FrameSyncAckPackage error!!!" << std::endl;
                    return;
                }

                clientFrame.OnAckReceived(ack);
            }
            break;

            case GSMT_FrameServerPackage:
            {
                ServerFramePackage response;
                if(!response.ParseFromString(msg))
                {
                    std::cout << "client onMessageCallBack parse GSMT_FrameServerPackage error!!!" << std::endl;
                    return;
                }

            
                FramePackage frame;
                if(!frame.ParseFromString(response.package()))
                {
                    std::cout << "client onMessageCallBack parse FramePackage error!!!" << std::endl;
                    return;
                }

                // 只处理发给自己的以及和自己相关的
                if(defaultPlayerID != response.msg_client_id())
                {
                    return;
                }

               
                 if(frame.inputs_size() > 0)
			    {
                    /*
				    std::cout << "OnMessage ServerFramePackage recvClientId:=" << response.msg_client_id() << std::endl;
					std::cout << std::endl << "frame_index:=" << frame.frame_index() << " timeStamp:=" << frame.timestamp_ms() << std::endl;
					for(int i = 0; i < frame.inputs_size(); ++i)
					{
						const ClientInput& input = frame.inputs(i);
						std::cout << "player_index:=" << input.player_id() << " x:=" << input.move_x() << " y:=" << input.move_y() << std::endl;
					}
                    */
			    }
            }
            break;
                
            default:
                break;
        }
    });

    client->Connect();
    

    // 启动 I/O 线程
    std::thread io_thread([&loop, connected]() {
        //std::cout << "io_thread thread_id:=" << std::this_thread::get_id() << std::endl;
        loop.Loop();
    });
    
    // 等待连接建立
    {
        std::unique_lock<std::mutex> lock(mtx);
        cv.wait(lock, [&] { return connected; });
    }


    // ClientPredictionManager clientFrame(defaultPlayerID, client->GetTcpConnection());
    clientFrame.InitTcpConnection(client->GetTcpConnection());
    std::atomic<bool> stopFrameWork = false;
    std::thread frameThread([&stopFrameWork, &clientFrame, &loop](){
        uint32_t client_frame = 0;
        while(false == stopFrameWork.load(std::memory_order_acquire))
        {
            // 每20毫秒执行一次tick
            std::this_thread::sleep_for(std::chrono::milliseconds(20));
            loop.RunInLoop([&clientFrame](){
                clientFrame.Tick(20);
            });
            
        }
    });



    while(true)
    {
        
        std::cout << "input Key(wsad):=";
        char ch;
        std::cin >> ch;
        if('Q' == ch || 'q' == ch)
        {
            break;
        }

        int moveX = 0;
        int moveY = 0;
        if('w' == ch || 'W' == ch)
        {
            moveY = 1;
        }
        else if('s' == ch || 'S' == ch)
        {
            moveY = -1;
        }
        else if('A' == ch || 'a' == ch)
        {
            moveX = -1;
        }
        else if('D' == ch || 'd' == ch)
        {
            moveX = 1;
        }
        else 
        {
            ;

        }

        std::cout << ch << std::endl;
        ClientInput input;
        input.set_player_id(1);
        input.set_move_x(moveX);
        input.set_move_y(moveY);
        input.set_attack(false);
        clientFrame.OnLocalInput(input);
    }
    

    std::this_thread::sleep_for(std::chrono::seconds(60));
    stopFrameWork.store(true, std::memory_order_release);
    
    std::cout << "end !!!!" << std::endl;
    if(frameThread.joinable())
    {
        frameThread.join();
    }

    loop.Quit();
    io_thread.join();
    //return 0;
}





