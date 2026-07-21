#pragma once
#include <array>
#include <unordered_map>
#include <cstdint>
#include <functional>
#include <string>
#include <atomic>
#include <memory>
#include <mutex>
#include <map>
#include "ClientSnapshot.h"
#include "../../Common/FixedPoint.h"
#include "../../Common/FixedPonitMaxFunc.h"
#include "../../../build/proto_gen/frame_sync.pb.h"

class TcpConnection;
class ClientPredictionManager
{
public:
    ClientPredictionManager(uint32_t local_player_id, std::shared_ptr<TcpConnection> tcpConnection);

    ~ClientPredictionManager();
    
    //------由客户端主循环调用(每帧)-------------
    // 1.处理本地输入: 立即预测，并缓存输入
    void OnLocalInput(const ClientInput& input);

    // 3.获取当前本地预测状态(用于渲染)
    PlayerSnapshot GetCurrentState() const { return local_state_; }

    // 4.每帧Tick(推进本地帧号，保存快照)
    void Tick(uint32_t delta_ms);

    // 测试临时使用
    void InitTcpConnection(std::shared_ptr<TcpConnection> tcpConnection);

    void OnAckReceived(const TestAckPackage& ack);

    void OnCorrection(const ServerCorrection& correction);

private:
    // 模拟函数：根据输入更新状态（先用简单移动）
    void Simulate(const ClientInput& input, float delta_ms);

    // 发送数据给服务端（网络层接口）
    void SendToSever(const ClientInput& input);

    void Rollback(uint32_t target_frame);

private:
    uint32_t local_player_id_;
    std::shared_ptr<TcpConnection> tcp_conn_;

    std::atomic<uint32_t> current_client_frame_ = 0;                 // 【来源：客户端】本地自增帧号
    PlayerSnapshot local_state_;                        // 当前预测状态

    // 【关键】待确认输入队列：key = 客户端预测帧号
    std::mutex mutex_;
    std::map<uint32_t, ClientInput> pending_inputs_;

    // std::unordered_map<uint32_t, uint32_t> client_to_server_map_; // 建立映射， 客户端的帧号所对应的服务器的帧号。


    SnapshotStorage snapshots_;  // 新增
};

