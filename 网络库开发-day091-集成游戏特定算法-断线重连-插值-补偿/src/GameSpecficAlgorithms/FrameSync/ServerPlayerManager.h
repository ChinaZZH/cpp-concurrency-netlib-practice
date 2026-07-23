// ServerPlayerManager.h
#pragma once
#include <unordered_map>
#include <cstdint>
#include <functional>
#include <string>
#include <atomic>
#include <array>
#include "../../Common/FixedPoint.h"
#include "../../Common/FixedPonitMaxFunc.h"
#include "../../../build/proto_gen/frame_sync.pb.h"
#include "../../../build/proto_gen/reconnect.pb.h"

// 玩家权威状态（服务端持有）
struct ServerPlayerState{
    Fixed x, y;             // 位置
    Fixed vx, vy;           // 速度
    int32_t state = 0;      // 0=待机, 1=跑动, 2=跳跃, 3=攻击...
    uint32_t hp = 100;      // 血量（如果需要回滚伤害）
};


// 【新增】历史条目
struct HistoryEntry
{
    uint32_t client_frame = 0;      // 客户端预测帧号（0 表示无效）
    uint32_t server_frame = 0;      // 服务端逻辑帧号（备用）
    ServerPlayerState state;        // 该帧的状态, 该帧的快照
    //bool valid = false;             // 是否有效
};

class ServerPlayerManager
{
public:
    using SendCorrectionCallback = std::function<void(uint32_t player_id, const std::string& data)>;

    ServerPlayerManager(SendCorrectionCallback cb);

    // 1. 注册玩家（当玩家加入游戏时调用）
    void AddPlayer(uint32_t player_id);

    // 2. 移除玩家（玩家退出时调用）
    void RemovePlayer(uint32_t player_id);
    
    // 3. 获取某个玩家的权威状态（用于下发给客户端）
    bool GetPlayerState(uint32_t player_id, ServerPlayerState& out) const;

    // 4. 核心：每帧 Tick，推进所有玩家的状态
    //    参数：当前服务端帧号，所有玩家在这一帧的输入列表，时间步长(ms)
    void Tick(uint32_t delta_ms);


    // 由 TcpServer 的 OnMessage 调用，将客户端上报的输入暂存起来
    void SumbitInput(uint32_t player_id, const ClientInput& input);


    // 【新增】构建全量快照回复包（只读）
    bool BuildSnapShotReply(uint32_t player_id, SnapshotReply& reply);
    
    const std::unordered_map<uint32_t, ServerPlayerState>& GetAllPlayersState() const { return players_; }
    
    // 按客户端帧号查询历史状态
    bool GetHistoryByServerFrame(uint32_t player_id, uint32_t server_frame_index, ServerPlayerState& out);

    uint32_t GetServerFrameIndex() { return server_frame_index_; }

private:
    // 内部模拟函数（与客户端完全一致） 
    ServerPlayerState Simulate(const ServerPlayerState& current, const ClientInput& input, uint32_t delta_ms);

    // 推入历史数据
    void PushHistory(uint32_t player_id, uint32_t client_frame, uint32_t server_frame, const ServerPlayerState& state);

private:
    // 存储所有玩家的权威状态
    std::unordered_map<uint32_t, ServerPlayerState>  players_;

    struct PendingInput {
        ClientInput input;
        uint32_t client_frame;
    };

    std::unordered_map<uint32_t, PendingInput>  pending_inputs_;

    SendCorrectionCallback send_correction_cb_;

    // 每个玩家的历史缓冲区(环形，容量64) 
    // 容量 64 帧（64 × 20ms = 1.28 秒），足以覆盖绝大部分玩家的网络延迟（通常 RTT < 200ms，预留 1 秒有余量）
    struct PlayerHistory
    {
        std::array<HistoryEntry, 64> buffer;
        uint32_t head = 0;          // 下一个写入位置
        uint32_t count = 0;         // 已存储条目数

        void Push(const HistoryEntry& entry)
        {
            buffer[head] = entry;
            head = (head + 1) % buffer.size();
            if(count < buffer.size())
            {
                count++;
            }
        }

        bool FindClientFrame(uint32_t server_frame_index, ServerPlayerState& out) const
        {
            if(count <= 0)
            {
                return false;
            }

            // 从后往前找。
            int32_t start_index = 0;
            if(0 == head)
            {
                start_index = buffer.size() - 1;
            }
            else
            {
                start_index = head - 1;
            }
           

            int num = 0;
            for(int index = start_index; num < count ; --index, ++num)
            {
                // 则需要将i转换为正常的index
                if(index < 0)
                {
                    index = buffer.size() + index;
                }

                const auto& entry = buffer[index];
                if(entry.server_frame == server_frame_index)
                {
                    out = entry.state;
                    return true;
                }
            }

            return false;
        }
    };

    std::unordered_map<uint32_t, PlayerHistory> histories_; // player_id -> 历史

    uint32_t server_frame_index_ = 0;
};