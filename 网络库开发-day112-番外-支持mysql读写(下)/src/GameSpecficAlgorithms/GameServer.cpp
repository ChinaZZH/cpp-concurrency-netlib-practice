#include "GameServer.h"
#include <sstream>
#include "../Common/LogFile.h"
#include "AOI/IAOIManager.h"
#include "AOI/BaseAOIManager.h"
#include "AOI/GridAOI.h"
#include "AOI/CrossListAOI.h"
#include "AOI/QuadTreeAOI.h"
#include "../TcpConnection.h"
#include "../EventLoop.h"
#include "../Decoder/LengthAndTypePrefixDecoder.h"
#include "../Http/SimpleHttpClient.h"
#include "../ServiceDiscovery/ServiceRegistry.h"
#include "AttributeSync/DeltaSyncManager.h"
#include "../../build/proto_gen/aoi.pb.h"
#include "../../build/proto_gen/frame_sync.pb.h"
#include "../../build/proto_gen/reconnect.pb.h"
#include "../../build/proto_gen/game_event.pb.h"
#include "FrameSync/InputBuffer.h"
#include "FrameSync/FrameScheduler.h"
#include "FrameSync/ServerPlayerManager.h"
#include "EventSink/EventSink.h"
#include "EventSink/EventStore.h"
#include "Match/MatchManager.h"
#include "DynamicPartition/PartitionManager.h"
#include "DynamicPartition/MigrationManager.h"
#include "../MySql/DBTask.h"
#include "../MySql/DBConnectionPool.h"
#include "../../build/proto_gen/mysql.pb.h"

GameServer::GameServer(EventLoop* loop, int nPort)
:server_(loop, nPort)
,service_registry_(std::make_unique<ServiceRegistry>())
,parititionedPool_(std::make_shared<PartitionedPool>())
,event_sink_ptr_(std::make_shared<EventSink>())
,event_store_ptr_(std::make_shared<EventStore>())
,match_mgr_(std::make_shared<MatchManager>())
,match_timer_id_(0)
 {
    server_.SetMessageCallBack(std::bind(&GameServer::OnMessage, 
        this, std::placeholders::_1, 
        std::placeholders::_2,
        std::placeholders::_3)
    );


    server_.SetConnectionCallBack([](const std::shared_ptr<TcpConnection>& con){
        auto length_decoder = std::make_unique<LengthAndTypePrefixDecoder>();
        con->SetDecoder(std::move(length_decoder));
    });    

    event_sink_ptr_->Init("../logs/events.bin");
    event_store_ptr_->Init(event_sink_ptr_);

    partition_mgr_ = std::make_shared<PartitionManager>();
    migration_mgr_ = std::make_shared<MigrationManager>(partition_mgr_);

    AABB world_bounds{0, 0, 1024, 1024};
    partition_mgr_->Init(world_bounds, 100);
    
    migration_mgr_->SetOnDataReadyCallback([](const MigrationData& data){
        // 发送给目标节点 消息类型 GSMT_MigrationData
        // 可以考虑各个游戏服务器都直连，然后存一个 服务器结点id->服务器info的映射
        // 或者所有的服务器服务器都连一个中心/中转 服务器，让中心/中转服务器进行转发
    });

    migration_mgr_->SetOnTargetMigrationAckCallback([](uint32_t src_server_node_id, uint32_t partition_id, bool success, std::string strErrorMsg){
         // 发送给客户端目前迁移完成的结果
        MigrationAck result_ack;
        result_ack.set_partition_id(partition_id);
        result_ack.set_success(success);
        result_ack.set_error_msg(strErrorMsg);

        std::string strAckInfo = result_ack.SerializeAsString();
        // 发送 GSMT_MigrationAck 给 src_server_node_id 服务器节点进程
        // this->SendMessage(src_server_node_id, strFrame, GSMT_FrameServerPackage);
    });

    migration_mgr_->SetOnMigrationCompleteCallback([](uint32_t partition_id, bool success, std::string strErrorMsg){
        // 发送给客户端目前迁移完成的结果
        MigrationAck result_ack;
        result_ack.set_partition_id(partition_id);
        result_ack.set_success(success);
        result_ack.set_error_msg(strErrorMsg);

        std::string strFrame = result_ack.SerializeAsString();
        // this->SendMessage(entityId, strFrame, GSMT_FrameServerPackage);
    });
 }
    

 GameServer::~GameServer()
 {
    if(parititionedPool_ && match_timer_id_ > 0)
    {
        int threadIdx = 100 % parititionedPool_->GetParitionedCount();
        parititionedPool_->CancelTimer(threadIdx, match_timer_id_);
        match_timer_id_ = 0;
    }


    {
        stop_frame_scheduler_flag_.store(false, std::memory_order_release);
    }

    if(frame_broadcast_thread_ && frame_broadcast_thread_->joinable())
    {
        frame_broadcast_thread_->join();
    }
 }


void GameServer::Start()
{
    // 启动的时候设置db任务线程池
    {
        std::unique_ptr<MySql::DBConnectionPool> db_connection_pool = std::make_unique<MySql::DBConnectionPool>();
        if(false == db_connection_pool->Init("127.0.0.1", "root", "zzh@890918", "game_server", 3306, 5)) {
            std::cout << "connection to mysql db error!!!" << std::endl;
            return;
        }
    
        db_thread_pool_ = std::make_unique<MySql::DBThreadPool>(std::move(db_connection_pool));
    }

    // 异步执行sql语句
    RegisterHandler(GSMT_AsyncDbRequest, std::bind(&GameServer::OnAsyncDbRequest, this, std::placeholders::_1, std::placeholders::_2));
    RegisterHandler(GSMT_SyncDbRequest, std::bind(&GameServer::OnSyncDbRequest, this, std::placeholders::_1, std::placeholders::_2));

    // 注册处理函数
    //std::cout << "GameServer::Start  1111" << std::endl;
    RegisterHandler(GSMT_AddEntity, std::bind(&GameServer::AddEntity, this, std::placeholders::_1, std::placeholders::_2));
    RegisterHandler(GSMT_RemoveEntity, std::bind(&GameServer::RemoveEntity, this, std::placeholders::_1, std::placeholders::_2));
    RegisterHandler(GSMT_MoveEntity, std::bind(&GameServer::MoveEntity, this, std::placeholders::_1, std::placeholders::_2));
    
    // 状态同步(属性同步)
    RegisterHandler(GSMT_NACK_REQUEST, std::bind(&GameServer::OnNackRequest, this, std::placeholders::_1, std::placeholders::_2));
    
    // 帧同步
    RegisterHandler(GSMT_FrameClientInput, std::bind(&GameServer::FrameClientInput, this, std::placeholders::_1, std::placeholders::_2));
    RegisterHandler(GSMT_FrameSyncAddPlayer, std::bind(&GameServer::OnFrameSyncAddPlayer, this, std::placeholders::_1, std::placeholders::_2));
    RegisterHandler(GSMT_FrameSyncRemovePlayer, std::bind(&GameServer::OnFrameSyncRemovePlayer, this, std::placeholders::_1, std::placeholders::_2));
    
    // 帧同步 断线重连
    RegisterHandler(GSMT_FrameReconnect, std::bind(&GameServer::OnFrameReconnect, this, std::placeholders::_1, std::placeholders::_2));

    // 帧同步 补偿
    RegisterHandler(GSMT_FrameAttackRequest, std::bind(&GameServer::OnFrameAttackRequest, this, std::placeholders::_1, std::placeholders::_2));

    // 动态分区
    RegisterHandler(GSMT_MigrationRequest, std::bind(&GameServer::OnMigrationRequest, this, std::placeholders::_1, std::placeholders::_2));
    RegisterHandler(GSMT_MigrationData, std::bind(&GameServer::OnMigrationData, this, std::placeholders::_1, std::placeholders::_2));
    RegisterHandler(GSMT_MigrationAck, std::bind(&GameServer::OnMigrationAck, this, std::placeholders::_1, std::placeholders::_2));

    // 设置分区线程池个数
    //std::cout << "GameServer::Start  22222" << std::endl;
    parititionedPool_->Start(std::thread::hardware_concurrency());

    //std::cout << "GameServer::Start  33333" << std::endl;
    IAOIManager::SendMsgCallBack funcCallback = std::bind(&GameServer::SendMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3);

    for(int idMap = 100; idMap < 200; idMap += 1)
    {
        auto ptrAoiMap = std::make_shared<QuadTreeAOI>(1000, 1000, 100, 2, 8); // 世界 1000x1000，网格 100
        {
            // 设置移动的距离超过阈值才广播
            int threadIdx = idMap % parititionedPool_->GetParitionedCount();
            int forceMoveMsgDelaySeconds = 1;
            ptrAoiMap->InitAoiData(threadIdx, MOVE_THRESHOLD, parititionedPool_, forceMoveMsgDelaySeconds);

            // 设置aoi回调函数
            ptrAoiMap->SetSendMessageCallBack(funcCallback);
        }
    
        aoiMap_[idMap] = ptrAoiMap;

        // 差值同步管理器
        auto delaSyncMgr_ = std::make_shared<DeltaSyncManager>(ptrAoiMap, funcCallback);
        deltaSyncManager_[idMap] = delaSyncMgr_;
    }

    
     // 帧同步
     {
        input_buffer_ = std::make_unique<InputBuffer>();

        server_player_mgr_ = std::make_unique<ServerPlayerManager>([this](uint32_t playerId, const std::string& data){
            this->SendMessage(playerId, data, GSMT_ServerCorrection);
        });

        server_player_mgr_->SetKickCallback([this](uint32_t player_id){
            this->KickPlayer(player_id);
        });
        
        server_player_mgr_->SetBanCallback([this](uint32_t player_id){
            this->KickPlayer(player_id);
            ban_player_id_list_.insert(player_id);
        });


        // 每帧50毫秒，相当于20fps
        frame_scheduler_ = std::make_unique<FrameScheduler>(input_buffer_.get(), server_player_mgr_.get(), [this](const std::string& serialized_pkg){
            for(auto& [entityId, tcpConnection] : onServerConnections_)
            {
                ServerFramePackage serverFrame;
                serverFrame.set_msg_client_id(entityId);
                serverFrame.set_package(serialized_pkg);
                //std::cout << "serialized_pkg size:=" << serialized_pkg.size() << std::endl; 
                std::string strFrame = serverFrame.SerializeAsString();
                this->SendMessage(entityId, strFrame, GSMT_FrameServerPackage);
            }
        }); 


        frame_broadcast_thread_ = std::make_unique<std::thread>([this](){
            uint32_t logicTickCount = 0;
            while(false == stop_frame_scheduler_flag_.load(std::memory_order_acquire))
            {
                std::this_thread::sleep_for(std::chrono::microseconds(10)); // 每次50毫秒tick一次，然后
                logicTickCount += 1;
                if(logicTickCount >= 5)
                {
                    frame_scheduler_->OnTick();
                    logicTickCount = 0;
                }

                std::this_thread::sleep_for(std::chrono::microseconds(10)); // 每次50毫秒tick一次，然后
                logicTickCount += 1;
                if(logicTickCount >= 5)
                {
                    frame_scheduler_->OnTick();
                    logicTickCount = 0;
                }

                server_player_mgr_->Tick(20);    // 每20毫毛tick一次
            }
        });
     }
    

    {
        match_mgr_->SetTimeoutCallback([this](uint32_t player_id){
            // 通知对方已经被移除了。
            std::string strData;
            this->SendMessage(player_id, strData, GSMT_RemovePlayerFromMatch);
        });


        match_mgr_->SetMatchSuccessCallback([](const std::vector<uint32_t>& vecPlayerIdList){
            // 配对成功：启动一个新对局
            /*
            uint32_t room_id = CreateRoom();
            for (uint32_t pid : players) {
                AssignPlayerToRoom(pid, room_id);
                // 通知客户端匹配成功，准备开始游戏
                SendMatchSuccess(pid, room_id);
            }

            StartGameLoop(room_id);
            */
        });


        int threadIdx = 100 % parititionedPool_->GetParitionedCount();
        match_timer_id_ = parititionedPool_->DelayRunEvery(threadIdx, 30, [this](){
            match_mgr_->Tick();
        });

    }

    // 动态分区 每40秒一个tick
    {
        int threadIdx = 100 % parititionedPool_->GetParitionedCount();
        match_timer_id_ = parititionedPool_->DelayRunEvery(threadIdx, 30, [this](){
            migration_mgr_->OnTimerTick();
        });
    }

    //std::cout << "GameServer::Start  44444" << std::endl;
    server_.Start();
    //std::cout << "GameServer::Start  55555" << std::endl;
}

 void GameServer::RegisterHandler(GameServerMsgType msgType, GameHandler handler)
 {
    methods_handler_[msgType] = handler;
 }


 void GameServer::EnableServiceDiscovery(const std::string& registry_host, int registry_port, 
        const std::string& service_name, const std::string& my_ip, int my_port, int ttl_sec)
{
    if(service_registry_)
    {
        service_registry_->EnableServiceDiscovery(registry_host, registry_port, service_name, my_ip, my_port, ttl_sec);
    }
}


void GameServer::OnMessage(const std::shared_ptr<TcpConnection>& con, std::string& strMsg, uint32_t msgType)
{
    std::weak_ptr<TcpConnection> weak_connection_ptr = con;
    auto MessageHandlerFunc = [this, strMsg=std::move(strMsg), msgType, weak_connection_ptr](){
            auto itr = methods_handler_.find(static_cast<GameServerMsgType>(msgType));
            if(itr == methods_handler_.end())
            {
                std::stringstream ss;
                ss << "GameServer::OnMessage not found handler msgtype:=" << msgType << std::endl;
                LogFile& logfile = LogFile::getInstance();
                logfile.AppendContent("GameServer_OnMessage.txt", ss.str());
                return;
            }


            try{
                GameHandler handler = (itr->second);
                bool result = handler(weak_connection_ptr, strMsg);
                if(!result)
                {
                    std::stringstream ss;
                    ss << "GameServer::OnMessage process handler false msgtype:=" << msgType << std::endl;
                    LogFile& logfile = LogFile::getInstance();
                    logfile.AppendContent("GameServer_OnMessage.txt", ss.str());   
                }
            }
            catch(const std::exception& e)
            {
                std::stringstream ss;
                ss << "GameServer::OnMessage process handler error msgtype:=" << msgType <<  " expection:=" << e.what() << std::endl;
                LogFile& logfile = LogFile::getInstance();
                logfile.AppendContent("GameServer_OnMessage.txt", ss.str());
            }
    };

    
    int mapId = this->TryExtractMapId(strMsg, msgType);
    if(mapId >= 0)
    {
        int idx = mapId % parititionedPool_->GetParitionedCount();
        parititionedPool_->CommitTask(idx, MessageHandlerFunc);
    }
    else
    {
        ThreadPool* task_thread_pool = server_.GetThreadPool(); 
        assert(task_thread_pool);

        task_thread_pool->AddTask(MessageHandlerFunc);
    }

}

bool GameServer::OnAsyncDbRequest(const std::weak_ptr<TcpConnection>& weak_connection_ptr, const std::string& strParamData)
{
    mysqlDb::AsyncDbRequest request;
    if(!request.ParseFromString(strParamData))
    {
        throw std::runtime_error("parse DbRequest failed");
    }  

    uint32_t query_player_id = request.player_id();
    auto HandleAsyncDbResponse = [this, query_player_id](const MySql::DBResult& result){
        if(false == result.success)
        {
            mysqlDb::DbResponse response;
            response.set_player_id(query_player_id);
            response.set_success_code(0);

            std::string strData;
            response.SerializeToString(&strData);
            this->SendMessage(query_player_id, strData, GSMT_RetDbResponse);
            return ;
        }

        // 查询成功，则判断是update/insert/delete
        if(result.affected_rows > 0){
            this->HandleAsyncDbUpdateResponse(query_player_id, result);
        }else{
            this->HandleAsyncDbSelectResponse(query_player_id, result);
        }
    };

    std::shared_ptr<MySql::DBTask> db_task = std::make_shared<MySql::DBTask>();
    db_task->sql = std::move(request.sql_context());
    db_task->conn_id = query_player_id;
    db_task->callback = HandleAsyncDbResponse;
    db_thread_pool_->SubmitTask(db_task);
    return true;
}


bool GameServer::OnSyncDbRequest(const std::weak_ptr<TcpConnection>& weak_connection_ptr, const std::string& strParamData)
{
    mysqlDb::DbSyncRequest request;
    if(!request.ParseFromString(strParamData))
    {
        throw std::runtime_error("parse DbRequest failed");
    }  

    uint32_t query_player_id = request.player_id();
    std::shared_ptr<MySql::DBTask> db_task = std::make_shared<MySql::DBTask>();
    db_task->sql = std::move(request.sql_context());
    db_task->conn_id = query_player_id;
    MySql::DBResult result = db_thread_pool_->ExecuteSync(db_task);
    if(false == result.success)
    {
        mysqlDb::DbResponse response;
        response.set_player_id(query_player_id);
        response.set_success_code(0);

        std::string strData;
        response.SerializeToString(&strData);
        this->SendMessage(query_player_id, strData, GSMT_RetDbResponse);
        return true;
    }

    // 查询成功，则判断是update/insert/delete
    if(result.affected_rows > 0){
        this->HandleAsyncDbUpdateResponse(query_player_id, result);
    }else{
        this->HandleAsyncDbSelectResponse(query_player_id, result);
    }

    return true;
}

bool GameServer::AddEntity(const std::weak_ptr<TcpConnection>& weak_connection_ptr, const std::string& strParamData)
{
    aoi::EntityEnterRequest request;
    if(!request.ParseFromString(strParamData))
    {
        throw std::runtime_error("parse AddEntity failed");
    }   

    
    auto itr = aoiMap_.find(request.map_id());
    if(itr == aoiMap_.end())
    {
        throw std::runtime_error("mapid error failed");
    }

    // 这边connection只是读取对应的eventLoop不会影响多线程的竞争问题
    const aoi::EntityInfo& entityInfo = request.new_entity();
     
    EventLoop* loop_ptr = nullptr;
    auto con = weak_connection_ptr.lock();
    if(con)
    {
        loop_ptr = con->GetLoop();
    }

    if(!loop_ptr)
    {
        throw std::runtime_error("loop_ptr nullptr error!!!");
    }

    TcpConnectionInfo connectionInfo;
    connectionInfo.weakPtrCon = weak_connection_ptr;
    connectionInfo.loop_ptr = loop_ptr;    
    onServerConnections_[entityInfo.entity_id()] = connectionInfo;


    bool bAddEntityResult = (itr->second)->AddEntity(entityInfo.entity_id(), entityInfo.x(), entityInfo.y());
    if(!bAddEntityResult)
    {
        onServerConnections_.erase(entityInfo.entity_id());
        return false;
    }
    
    PrintNeighbors((itr->second), 1);
    return true;
}

bool GameServer::RemoveEntity(const std::weak_ptr<TcpConnection>& weak_connection_ptr, const std::string& strParamData)
{
    aoi::EntityLeaveRequest request;
    if(!request.ParseFromString(strParamData))
    {
        throw std::runtime_error("parse RemoveEntity failed");
    }   

    auto itr = aoiMap_.find(request.map_id());
    if(itr == aoiMap_.end())
    {
        throw std::runtime_error("mapid error failed");
    }

    bool bAddEntityResult = (itr->second)->RemoveEntity(request.entity_id());
    if(!bAddEntityResult)
    {
        return false;
    }

    onServerConnections_.erase(request.entity_id());
    PrintNeighbors((itr->second), 1);
    return true;
}
    
bool GameServer::MoveEntity(const std::weak_ptr<TcpConnection>& weak_connection_ptr, const std::string& strParamData)
{
    aoi::EntityMoveRequest moveRequest;
    if(!moveRequest.ParseFromString(strParamData))
    {
        throw std::runtime_error("GameServer::MoveEntity parse MoveEntity failed");
    } 

    // 获取实体
    auto itr = aoiMap_.find(moveRequest.map_id());
    if(itr == aoiMap_.end())
    {
        throw std::runtime_error("GameServer::MoveEntity mapid error failed");
    }
    
    // 计算移动距离， 通过两点式计算距离
    int entityId = moveRequest.entity_id();
    int newX = moveRequest.new_x();
    int newY =  moveRequest.new_y();
    std::shared_ptr<IAOIManager> aoiManager = (itr->second);
    EntityPositionResult posResult = aoiManager->GetEntityPosition(entityId);
    if(false == posResult.valid)
    {
        throw std::runtime_error("GameServer::MoveEntity not found entity");
    }

    
    int deltaX = posResult.x - newX;
    int deltaY = posResult.y - newY;
    int squareDistance = (deltaX * deltaX) + (deltaY * deltaY);
    float distance = sqrt(squareDistance);
    

    // 4. 校验：速度限制 + 防闪现  这边使用秒做计算，需要的时候再调整精度到毫秒或者微秒
    {
        // 校验速度是否超过限制
        auto now = std::chrono::steady_clock::now();
        auto deltaSecs = std::chrono::duration<float>(now - posResult.lastUpdateTime).count();
        if(deltaSecs > 0.00f && (distance / deltaSecs) > MAX_MOVE_SPEED)
        {
            std::cout << "GameServer::MoveEntity entityId:=" << entityId << " out of speed range speed:=" << MAX_MOVE_SPEED << " client speed:=" << (distance / deltaSecs) << std::endl;
            return false;
        }
    }
    

    // 校验是否闪现
    {
        if(distance > MAX_TELEPORT_DIST)
        {
            std::cout << "GameServer::MoveEntity entityId:=" << entityId << " out of distance:=" << MAX_TELEPORT_DIST << " client distance:=" << distance << std::endl;
            return false;
        }
    }
    

    bool bAddEntityResult = (itr->second)->MoveEntity(entityId, newX, newY);
    if(!bAddEntityResult)
    {
        return false;
    }

    PrintNeighbors((itr->second), 1);
    return true;
}


bool GameServer::OnNackRequest(const std::weak_ptr<TcpConnection>& weak_connection_ptr, const std::string& strParamData)
{
    NackRequest req;
    if(!req.ParseFromString(strParamData))
    {
        throw std::runtime_error("GameServer::NackRequest parse MoveEntity failed");
    }

     // 获取实体
    auto itr = deltaSyncManager_.find(100);
    if(itr == deltaSyncManager_.end())
    {
        throw std::runtime_error("GameServer::NackRequest mapid error failed");
    }

    auto deltaSync = (itr->second);
    deltaSync->OnNackRequest(req.entity_id(), req.entity_id(), req.from_version());
    return true;
}


// 客户端发消息上来则往inputBuffer里面塞数据
bool GameServer::FrameClientInput(const std::weak_ptr<TcpConnection>& weak_connection_ptr, const std::string& strParamData)
{
    ClientInput req;
    if(!req.ParseFromString(strParamData))
    {
        throw std::runtime_error("GameServer::FrameClientInput parse ClientInput failed");
    }

    auto itr = onServerConnections_.find(req.player_id());
    if(itr == onServerConnections_.end())
    {
        // 这边connection只是读取对应的eventLoop不会影响多线程的竞争问题
        EventLoop* loop_ptr = nullptr;
        auto con = weak_connection_ptr.lock();
        if(con)
        {
            loop_ptr = con->GetLoop();
        }

        if(!loop_ptr)
        {
            throw std::runtime_error("loop_ptr nullptr error!!!");
        }

        TcpConnectionInfo connectionInfo;
        connectionInfo.weakPtrCon = weak_connection_ptr;
        connectionInfo.loop_ptr = loop_ptr;    
        onServerConnections_[req.player_id()] = connectionInfo;
    }

    server_player_mgr_->SumbitInput(req.player_id(), req);
    input_buffer_->PushInput(req.player_id(), frame_scheduler_->GetServerFrameIndex(), req);
    return true;
}

bool GameServer::OnFrameSyncAddPlayer(const std::weak_ptr<TcpConnection>& weak_connection_ptr, const std::string& strParamData)
{
    FrameSyncAddPlayer req;
    if(!req.ParseFromString(strParamData))
    {
        throw std::runtime_error("GameServer::FrameSyncAddPlayer parse ClientInput failed");
    }

    // 判断是否在黑名单内
    auto itrBan = ban_player_id_list_.find(req.player_id());
    if(itrBan != ban_player_id_list_.end())
    {
        return false;
    }

    auto itr = onServerConnections_.find(req.player_id());
    if(itr == onServerConnections_.end())
    {
        // 这边connection只是读取对应的eventLoop不会影响多线程的竞争问题
        EventLoop* loop_ptr = nullptr;
        auto con = weak_connection_ptr.lock();
        if(con)
        {
            loop_ptr = con->GetLoop();
        }

        if(!loop_ptr)
        {
            throw std::runtime_error("loop_ptr nullptr error!!!");
        }

        TcpConnectionInfo connectionInfo;
        connectionInfo.weakPtrCon = weak_connection_ptr;
        connectionInfo.loop_ptr = loop_ptr;    
        onServerConnections_[req.player_id()] = connectionInfo;
    }

    server_player_mgr_->AddPlayer(req.player_id());
    return true;
}

bool GameServer::OnFrameSyncRemovePlayer(const std::weak_ptr<TcpConnection>& weak_connection_ptr, const std::string& strParamData)
{
    FrameSyncRemovePlayer req;
    if(!req.ParseFromString(strParamData))
    {
        throw std::runtime_error("GameServer::FrameSyncRemovePlayer parse ClientInput failed");
    }

    onServerConnections_.erase(req.player_id());
    server_player_mgr_->RemovePlayer(req.player_id());
    return true;
}


bool GameServer::OnFrameReconnect(const std::weak_ptr<TcpConnection>& weak_connection_ptr, const std::string& strParamData)
{
    ReconnectRequest req;
    if(!req.ParseFromString(strParamData))
    {
        throw std::runtime_error("GameServer::OnFrameReconnect parse ClientInput failed");
    }

    SnapshotReply reply;
    if(server_player_mgr_->BuildSnapShotReply(req.player_id(), reply))
    {
        std::string strData;
        reply.SerializeToString(&strData);
        this->SendMessage(req.player_id(), strData, GSMT_FrameReconnect);
    }
    
    return true;
}

/*
消除网络延迟带来的“瞄准偏差”：玩家在客户端看到目标在位置 A，开了一枪。但等这个攻击包到达服务器时，

目标可能已经跑到位置 B。用历史位置判定，能让子弹打在“玩家看到的位置”上。

提升射击手感：这是 FPS 和 MOBA 游戏的标准做法（如《CS:GO》的 cl_interp 机制、《英雄联盟》的技能判定）。
*/

 bool GameServer::OnFrameAttackRequest(const std::weak_ptr<TcpConnection>& weak_connection_ptr, const std::string& strParamData)
 {
    AttackRequest req;
    if(!req.ParseFromString(strParamData))
    {
        throw std::runtime_error("GameServer::OnFrameAttackRequest parse ClientInput failed");
    }

    uint32_t player_id = req.player_id();
    uint32_t server_frame_index = req.server_frame_index();
    uint32_t target_id = req.target_id();

    // 将定点数原始值还原为 Fixed 对象
    Fixed dir_x = Fixed::FromRaw(req.dir_x());
    Fixed dir_y = Fixed::FromRaw(req.dir_y());

    uint32_t skill_id = req.skill_id();
    
    // 记录攻击事件
    {
        GameEvent event;
        event.set_player_id(player_id);
        event.set_server_frame(server_frame_index);
        event.set_timestamp_ms(GetCurrentTimeMs());
        event.set_event_type(EVENT_ATTACK);

        AttackPayload* attackPayload = event.mutable_attack();
        attackPayload->set_target_id(target_id);
        attackPayload->set_skill_id(skill_id);
        attackPayload->set_dir_x(req.dir_x());
        attackPayload->set_dir_y(req.dir_y());

        event_store_ptr_->Record(event);
    }

    // ================================================================
    // 0. 攻击频率校验
    // ================================================================
    bool bCheckAttack = server_player_mgr_->CheckAttackTarget(player_id, server_frame_index, target_id);
    if(false == bCheckAttack)
    {
       return false;
    }

    // ================================================================
    // 1. 获取攻击者的历史位置（自身位置）
    // ================================================================
    ServerPlayerState attacker_state;
    bool attacer_found = server_player_mgr_->GetHistoryByServerFrame(player_id, server_frame_index, attacker_state);
    if(!attacer_found)
    {
        // 如果攻击者自身的历史也查不到（极端情况：帧号太旧），使用实时位置
        server_player_mgr_->GetPlayerState(player_id, attacker_state);
        printf("[Server] Attacker history not found, using real-time position.\n");
    }


    // ================================================================
    // 2. 获取目标的历史位置（关键：延迟补偿）
    // ================================================================
    ServerPlayerState target_state;
    bool target_found = server_player_mgr_->GetHistoryByServerFrame(target_id, server_frame_index, target_state);
    if(!target_found)
    {
        // 如果攻击者自身的历史也查不到（极端情况：帧号太旧），使用实时位置
        server_player_mgr_->GetPlayerState(target_id, target_state);
        printf("[Server] Target history not found, using real-time position.\n");
    }

    // ================================================================
    // 3. 碰撞检测（使用历史位置）
    // ================================================================
    bool hit = false;
    Fixed attack_range = Fixed::FromRaw(5000);  // 攻击范围（示例：约 0.0045 单位，实际值根据游戏调）

    // 计算攻击者到目标的距离
    Fixed dx = target_state.x - attacker_state.x;
    Fixed dy = target_state.y - attacker_state.y;
    Fixed dist_sq = dx * dx + dy * dy;          // 距离平方（避免开方）

    // 攻击范围平方
    Fixed range_sq = attack_range * attack_range;

    if (dist_sq <= range_sq) {
        hit = true;
        printf("[Server] HIT! Attacker %u hit target %u at frame %u\n",
               player_id, target_id, server_frame_index);
    } else {
        printf("[Server] MISS. Distance=%.2f, Range=%.2f\n",
               FixedMath::FixedSqrt(dist_sq).ToDouble(), attack_range.ToDouble());
    }

    // ================================================================
    // 4. 下发命中结果（给攻击者和被攻击者）
    // ================================================================
    if (hit) {
        // 构造命中消息（假设有 HitResult 协议）
        HitResult result;
        result.set_attacker_id(player_id);
        result.set_target_id(target_id);
        result.set_damage(10);  // 示例伤害
        result.set_hit(true);

        std::string data;
        result.SerializeToString(&data);

        // 发给攻击者
        std::string strData;
        result.SerializeToString(&strData);
        this->SendMessage(player_id, strData, GSMT_FrameReconnect);

        // 发给被攻击者
        this->SendMessage(target_id, strData, GSMT_FrameReconnect);


        // 记录命中事件
        {
        
            GameEvent event;
            event.set_player_id(player_id);
            event.set_server_frame(server_frame_index);
            event.set_timestamp_ms(GetCurrentTimeMs());
            event.set_event_type(EVENT_HIT);

            HitPayload* hitPayload = event.mutable_hit();
            hitPayload->set_target_id(target_id);
            hitPayload->set_damage(10);
            hitPayload->set_remaining_hp(10);
        
            event_store_ptr_->Record(event);
        }
    }


    return true;
 }


uint32_t GameServer::GetCurrentTimeMs()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}


void GameServer::SetHp(int entityId, int64_t newHp)
{
    int32_t idMap = 100;
    auto itr = deltaSyncManager_.find(idMap);
    if(itr == deltaSyncManager_.end())
    {
        throw std::runtime_error("GameServer::SetHp mapid error failed");
    }

    auto deltaSync = (itr->second);
    int threadIdx = idMap % parititionedPool_->GetParitionedCount();
    parititionedPool_->CommitTask(threadIdx, [deltaSync, entityId, newHp](){
        deltaSync->OnAttributeChanged(entityId, 1, newHp);
    });
}

void GameServer::SendMessage(int entityId, const std::string& strMsgContent, GameServerMsgType msgType)
{
    // 不在线，已经离开了。
    auto itr = onServerConnections_.find(entityId);
    if(itr == onServerConnections_.end())
    {
        return ;
    }

    // 则发送数据到对应的客户端
    const TcpConnectionInfo& connectionInfo = (itr->second);
    std::string strContent = std::move(LengthAndTypePrefixDecoder::MakeRequestString(strMsgContent, msgType));
    connectionInfo.loop_ptr->RunInLoop([weak_con = connectionInfo.weakPtrCon, strContent=std::move(strContent)](){
        auto con = weak_con.lock();
        if(con)
        {
            con->Send(strContent);
        }
    });
}


int GameServer::TryExtractMapId(const std::string& strMsg, uint32_t msgType) 
{
    return 100;
    /*
    switch (static_cast<GameServerMsgType>(msgType)) {
        case GSMT_AddEntity: {
            aoi::EntityEnterRequest req;
            if (req.ParseFromString(strMsg)) 
            {
                return req.map_id();
            }

            break;
        }
        case GSMT_RemoveEntity: {
            aoi::EntityLeaveRequest req;
            if (req.ParseFromString(strMsg))
            {
                return req.map_id();
            } 

            break;
        }
        case GSMT_MoveEntity: {
            aoi::EntityMoveRequest req;
            if (req.ParseFromString(strMsg)) 
            {
                return req.map_id();
            }

            break;
        }
        default:
            break;
    }
    

    return -1;  // 无 mapId 或解析失败
    */
}


void GameServer::PrintNeighbors(std::shared_ptr<IAOIManager> aoi, int id)
{
    auto neighbors = aoi->GetNeighbors(id);
    std::cout << "Entity " << id << " neighbors: ";
    for (int n : neighbors) std::cout << n << " ";
    std::cout << std::endl;
}


void GameServer::KickPlayer(uint32_t player_id)
{
    auto itr = onServerConnections_.find(player_id);
    if(itr == onServerConnections_.end())
    {
        return ;
    }

    // 则发送数据到对应的客户端
    const TcpConnectionInfo& connectionInfo = (itr->second);
    connectionInfo.loop_ptr->RunInLoop([weak_con = connectionInfo.weakPtrCon](){
        auto con = weak_con.lock();
        if(con)
        {
            con->Shutdown();
        }
    });
}


bool GameServer::OnMigrationRequest(const std::weak_ptr<TcpConnection>& weak_connection_ptr, const std::string& strParamData)
{
    MigrationRequest request;
    if(false == request.ParseFromString(strParamData))
    {
        return false;
    }

    return migration_mgr_->StartMigration(request.partition_id(), request.target_node_id());
}

bool GameServer::OnMigrationData(const std::weak_ptr<TcpConnection>& weak_connection_ptr, const std::string& strParamData)
{
    MigrationData data;
    if(false == data.ParseFromString(strParamData))
    {
        return false;
    }

    return migration_mgr_->ReceiveMigrationData(data);
}

bool GameServer::OnMigrationAck(const std::weak_ptr<TcpConnection>& weak_connection_ptr, const std::string& strParamData)
{
    MigrationAck ack;
    if(false == ack.ParseFromString(strParamData))
    {
        return false;
    }

    uint32_t partition_id = ack.partition_id();
    bool on_ack_result = false;
    if(ack.success())
    {
        on_ack_result = migration_mgr_->ConfirmMigration(partition_id);
    }
    else{
        on_ack_result = migration_mgr_->RollbackMigration(partition_id);
    }

    return on_ack_result;
}



bool GameServer::HandleAsyncDbSelectResponse(uint32_t player_id, const MySql::DBResult& result)
{
    mysqlDb::DbResponse response;
    response.set_player_id(player_id);
    response.set_success_code(1);
    response.set_affect_rows(0);
    for(const auto& column_value : result.columns)
    {
        response.add_columns(column_value);
    }

    for(const auto& row_data : result.rows)
    {
        mysqlDb::DbFieldValueSet* pFieldValueSet = response.add_rows();
        for(const auto& field_value : row_data)
        {
            pFieldValueSet->add_field_value(field_value);
        }
    }
    

    std::string strData;
    response.SerializeToString(&strData);
    this->SendMessage(player_id, strData, GSMT_RetDbResponse);
    return true;
}
    
bool GameServer::HandleAsyncDbUpdateResponse(uint32_t player_id, const MySql::DBResult& result)
{
    mysqlDb::DbResponse response;
    response.set_player_id(player_id);
    response.set_success_code(1);
    response.set_affect_rows(result.affected_rows);

    std::string strData;
    response.SerializeToString(&strData);
    this->SendMessage(player_id, strData, GSMT_RetDbResponse);
    return true;
}
 