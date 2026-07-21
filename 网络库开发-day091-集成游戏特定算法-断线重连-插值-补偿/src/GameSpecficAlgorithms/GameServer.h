#pragma once
#include "../TcpServer.h"
#include <unordered_map>
#include <functional>
#include <string>
#include <fstream>
#include <thread>
#include <memory>
#include <atomic>
#include <absl/container/flat_hash_map.h>
#include "PartitionedPool.h"
#include "GameServerMsgTypeDefine.h"


class EventLoop;
class TcpConnection;
class IAOIManager;
class ServiceRegistry;
class DeltaSyncManager;
class InputBuffer;
class FrameScheduler;
class ServerPlayerManager;

class GameServer 
{
public:
    using GameHandler = std::function<bool(const std::weak_ptr<TcpConnection>& , const std::string&)>;

    GameServer(EventLoop* loop, int nPort);
    
    ~GameServer();

    void Start();

    void OnMessage(const std::shared_ptr<TcpConnection>& con, std::string& strMsg, uint32_t msgType);

    void SendMessage(int entityId, const std::string& strMsgContent, GameServerMsgType msgType);

    void RegisterHandler(GameServerMsgType msgType, GameHandler handler);

     void EnableServiceDiscovery(const std::string& registry_host, int registry_port, 
        const std::string& service_name, const std::string& my_ip, int my_port, int ttl_sec);
        
public:
    bool AddEntity(const std::weak_ptr<TcpConnection>& weak_connection_ptr, const std::string& strParamData);

    bool RemoveEntity(const std::weak_ptr<TcpConnection>& weak_connection_ptr, const std::string& strParamData);
    
    bool MoveEntity(const std::weak_ptr<TcpConnection>& weak_connection_ptr, const std::string& strParamData);
    
    bool OnNackRequest(const std::weak_ptr<TcpConnection>& weak_connection_ptr, const std::string& strParamData);

    bool FrameClientInput(const std::weak_ptr<TcpConnection>& weak_connection_ptr, const std::string& strParamData);

    bool OnFrameSyncAddPlayer(const std::weak_ptr<TcpConnection>& weak_connection_ptr, const std::string& strParamData);

    bool OnFrameSyncRemovePlayer(const std::weak_ptr<TcpConnection>& weak_connection_ptr, const std::string& strParamData);

    void SetHp(int entityId, int64_t newHp);

private:
    int TryExtractMapId(const std::string& strMsg, uint32_t msgType);

    void PrintNeighbors(std::shared_ptr<IAOIManager> aoi, int id);

private:
    // 服务器的基础设施 开始
    TcpServer server_;
    std::unique_ptr<ServiceRegistry> service_registry_;

    // 存储地图上的连接信息 
    struct TcpConnectionInfo
    {
        std::weak_ptr<TcpConnection> weakPtrCon;
        EventLoop* loop_ptr;
    };

    std::unordered_map<int, TcpConnectionInfo> onServerConnections_;

    absl::flat_hash_map<GameServerMsgType, GameHandler> methods_handler_;

    std::shared_ptr<PartitionedPool> parititionedPool_; // 根据地图id分区线程池
    // 服务器的基础设施，结束

    // aoi算法
    std::map<int, std::shared_ptr<IAOIManager>> aoiMap_; // 默认只有一张地图 aoi算法

    // 状态同步 属性同步， 差量值和全量值
    std::map<int, std::shared_ptr<DeltaSyncManager>> deltaSyncManager_; // 默认只有一张地图(状态同步 属性同步)


    // 状态同步， 位置同步
    const static int    MAX_MOVE_SPEED          = 50;        // 单位/秒
    const static int    MAX_TELEPORT_DIST       = 100;       // 防闪现阈值
    const static int    MOVE_THRESHOLD          = 10;        // 移动阈值（距离小于此值不广播）


    // 帧同步
    std::unique_ptr<InputBuffer> input_buffer_;
    std::unique_ptr<FrameScheduler> frame_scheduler_;
    std::unique_ptr<ServerPlayerManager> server_player_mgr_;

    std::unique_ptr<std::thread> frame_broadcast_thread_;
    std::atomic<bool> stop_frame_scheduler_flag_ = false;
};