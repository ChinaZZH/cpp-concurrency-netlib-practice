
// EventStore.h

#pragma once
#include <atomic>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <deque>
#include <map>
#include <vector>
#include <memory>
#include "EventSink.h"
#include "../../../build/proto_gen/game_event.pb.h"

// 事件存储 + 索引查询
class EventStore 
{
public:
    EventStore();

    ~EventStore() = default;

public:
    // 初始化：设置最大内存事件数，绑定 EventSink
    void Init(std::shared_ptr<EventSink> sink, size_t max_events = 5000);

    // 记录事件(线程安全)
    void Record(const GameEvent& event);

    // ----- 查询接口（线程安全）----

    // 按玩家 ID 查询最近 N 个事件
    std::vector<GameEvent> QueryByPlayer(uint32_t player_id, uint32_t limit = 20) const;

    // 按帧号范围查询事件([start_frame, end_frame])
    std::vector<GameEvent> QueryByFrameRange(uint32_t start_frame, uint32_t end_frame) const;

    // 获取当前内存中的事件总数
    size_t GetEventCount() const;

private:
    // 事件ID 类型
    using EventId = uint64_t;

    // 主存储：deque 存储事件，尾部追加，头部淘汰
    std::map<EventId, GameEvent> events_;

    // 玩家索引：player_id -> deque<EventId>（最近的在尾部）
    std::unordered_map<uint32_t, std::vector<EventId>> player_index_;

    // 帧号索引：server_frame -> vector<EventId>
    std::map<uint32_t, std::vector<EventId>> frame_index_;
     
     // 下一个事件 ID
     EventId next_id = 0;

     // 内存限制
     size_t max_events_ = 5000;

     // 持久化 sink
     std::shared_ptr<EventSink> sink_;

     // 线程安全锁
     mutable std::mutex mutex_;
};
