#pragma once
#include <memory>
#include <string>
#include "PartitionedPool.h"
#include "GameServerMsgTypeDefine.h"
#include "../LibRpc/RpcServer.h"

class EventLoop;
class TcpConnection;
class IAOIManager;

class GameServer : public RpcServer
{
public:
    using GameHandler = std::function<bool(const std::weak_ptr<TcpConnection>& , const std::string&)>;

    GameServer(EventLoop* loop, int nPort);
    
    ~GameServer();

    void OnMessage(const std::shared_ptr<TcpConnection>& con, std::string& strMsg, uint32_t msgType);

    void SendMessage(int entityId, const std::string& strMsgContent, GameServerMsgType msgType);

    void RegisterHandler(GameServerMsgType msgType, GameHandler handler);

public:
    bool AddEntity(const std::weak_ptr<TcpConnection>& weak_connection_ptr, const std::string& strParamData);

    bool RemoveEntity(const std::weak_ptr<TcpConnection>& weak_connection_ptr, const std::string& strParamData);
    
    bool MoveEntity(const std::weak_ptr<TcpConnection>& weak_connection_ptr, const std::string& strParamData);
    
private:
    int TryExtractMapId(const std::string& strMsg, uint32_t msgType);

    void PrintNeighbors(std::shared_ptr<IAOIManager> aoi, int id);

private:
    std::map<int, std::shared_ptr<IAOIManager>> aoiMap_; // 默认只有一张地图

    // 存储地图上的连接信息
    struct TcpConnectionInfo
    {
        std::weak_ptr<TcpConnection> weakPtrCon;
        EventLoop* loop_ptr;
    };

    std::unordered_map<int, TcpConnectionInfo> onServerConnections_;

    absl::flat_hash_map<GameServerMsgType, GameHandler> methods_handler_;

    std::unique_ptr<PartitionedPool> parititionedPool_; // 根据地图id分区线程池
};