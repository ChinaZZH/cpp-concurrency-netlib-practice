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
#include "../../build/proto_gen/aoi.pb.h"
#include "../Http/SimpleHttpClient.h"
#include "../ServiceDiscovery/ServiceRegistry.h"

 GameServer::GameServer(EventLoop* loop, int nPort)
:server_(loop, nPort)
,service_registry_(std::make_unique<ServiceRegistry>())
 ,parititionedPool_(std::make_shared<PartitionedPool>())
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
 }
    

 GameServer::~GameServer()
 {

 }


void GameServer::Start()
{
    // 注册处理函数
    //std::cout << "GameServer::Start  1111" << std::endl;
    RegisterHandler(GSMT_AddEntity, std::bind(&GameServer::AddEntity, this, std::placeholders::_1, std::placeholders::_2));
    RegisterHandler(GSMT_RemoveEntity, std::bind(&GameServer::RemoveEntity, this, std::placeholders::_1, std::placeholders::_2));
    RegisterHandler(GSMT_MoveEntity, std::bind(&GameServer::MoveEntity, this, std::placeholders::_1, std::placeholders::_2));


    // 设置分区线程池个数
    //std::cout << "GameServer::Start  22222" << std::endl;
    parititionedPool_->Start(std::thread::hardware_concurrency());

    //std::cout << "GameServer::Start  33333" << std::endl;
    for(int idMap = 100; idMap < 200; idMap += 1)
    {
        auto ptrAoiMap = std::make_shared<QuadTreeAOI>(1000, 1000, 100, 2, 8); // 世界 1000x1000，网格 100

        // 设置移动的距离超过阈值才广播
        int threadIdx = idMap % parititionedPool_->GetParitionedCount();
        int forceMoveMsgDelaySeconds = 1;
        ptrAoiMap->InitAoiData(threadIdx, MOVE_THRESHOLD, parititionedPool_, forceMoveMsgDelaySeconds);

        // 设置aoi回调函数
        ptrAoiMap->SetSendMessageCallBack(std::bind(&GameServer::SendMessage, this, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3));

        aoiMap_[idMap] = ptrAoiMap;
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
        /*
        auto now = std::chrono::steady_clock::now();
        auto deltaSecs = std::chrono::duration<float>(now - posResult.lastUpdateTime).count();
        if(deltaSecs > 0.00f && (distance / deltaSecs) > MAX_MOVE_SPEED)
        {
            std::cout << "GameServer::MoveEntity entityId:=" << entityId << " out of speed range speed:=" << MAX_MOVE_SPEED << " client speed:=" << (distance / deltaSecs) << std::endl;
            return false;
        }
        */
    }
    

    // 校验是否闪现
    {
        /*
        if(distance > MAX_TELEPORT_DIST)
        {
            std::cout << "GameServer::MoveEntity entityId:=" << entityId << " out of distance:=" << MAX_TELEPORT_DIST << " client distance:=" << distance << std::endl;
            return false;
        }
        */
    }
    

    bool bAddEntityResult = (itr->second)->MoveEntity(entityId, newX, newY);
    if(!bAddEntityResult)
    {
        return false;
    }

    PrintNeighbors((itr->second), 1);
    return true;
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
}


void GameServer::PrintNeighbors(std::shared_ptr<IAOIManager> aoi, int id)
{
    auto neighbors = aoi->GetNeighbors(id);
    std::cout << "Entity " << id << " neighbors: ";
    for (int n : neighbors) std::cout << n << " ";
    std::cout << std::endl;
}