#pragma once
#include <array>
#include <unordered_map>
#include <cstdint>
#include "../../Common/FixedPoint.h"
#include "../../Common/FixedPonitMaxFunc.h"



// 玩家状态快照（仅包含用于物理/移动判定的核心属性）
struct PlayerSnapshot{
    uint32_t current_frame_index = 0;      // 对应服务端帧号（用于回滚锚点）
    Fixed x, y;             // 位置
    //Fixed vx, vy;           // 速度
    //int32_t state = 0;      // 0=待机, 1=跑动, 2=跳跃, 3=攻击...
    //uint32_t hp = 100;      // 血量（如果需要回滚伤害）

     // 深拷贝默认即可，全是 POD 类型
};

// 环形快照缓冲区（容量 256 帧，约 5 秒 @ 50fps）
// 【关键】所有索引均使用服务端帧号 server_frame
class SnapshotStorage
{
public:
    static constexpr size_t CAPACITY = 65535;

    void Save(uint32_t current_frame_index, const PlayerSnapshot& snapshot)
    {
        size_t index = current_frame_index % CAPACITY;

        // 清理被覆盖的旧帧映射
        uint32_t old_frame = buffer_[index].current_frame_index;
        index_map_.erase(old_frame);


        // 2. 写入新快照
        buffer_[index] = snapshot;
        // 确保快照内部也记录正确的帧号
        buffer_[index].current_frame_index = current_frame_index;
        index_map_[current_frame_index] = index;
    }

    bool Restore(uint32_t client_frame, PlayerSnapshot& outSnapshot)
    {
        auto itr = index_map_.find(client_frame);
        if(itr == index_map_.end())
        {
            return false;
        }

        size_t idx = (itr->second);
        const auto& snap = buffer_[idx];
        if(snap.current_frame_index != client_frame)
        {
            return false;
        }

        outSnapshot = snap;
        return true;
    }

    // 清空（用于断线重连重置 重新读取全量数据，这个不需要缓存了）
    void Clear()
    {
        index_map_.clear();
        
        // 重置buff的帧号，防止旧数据残留
        for(auto& snap : buffer_)
        {
            snap.current_frame_index = 0;
        }
    }

private:
    std::array<PlayerSnapshot, CAPACITY>        buffer_;
    std::unordered_map<uint32_t, size_t>        index_map_; // frame -> buffer index
};