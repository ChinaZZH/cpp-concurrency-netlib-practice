#include "EventStore.h"
#include <algorithm>

EventStore::EventStore() = default;


// 初始化：设置最大内存事件数，绑定 EventSink
void EventStore::Init(std::shared_ptr<EventSink> sink, size_t max_events /*= 5000*/)
{
    sink_ = sink;
    max_events_ = max_events;
}

// 记录事件(线程安全)
void EventStore::Record(const GameEvent& event)
{
    std::lock_guard<std::mutex> lk(mutex_);
    next_id += 1;
    events_[next_id] = event;

    auto& vecPlayerEvent = player_index_[event.player_id()];
    vecPlayerEvent.push_back(next_id);

    auto& vecFrameEvent = frame_index_[event.server_frame()];
    vecFrameEvent.push_back(next_id);

    while(events_.size() > max_events_)
    {
        auto itr_delete = events_.begin();
        EventId deleteEventId = itr_delete->first;
        GameEvent deleteEvent = std::move(itr_delete->second);
        events_.erase(itr_delete);

        auto& vecPlayerEvent = player_index_[deleteEvent.player_id()];
        std::erase(vecPlayerEvent, deleteEventId);
        if(vecPlayerEvent.empty())
        {
            player_index_.erase(deleteEvent.player_id());
        }

        auto& vecFrameEvent = frame_index_[deleteEvent.server_frame()];
        std::erase(vecFrameEvent, deleteEventId);
        if(vecFrameEvent.empty())
        {
            frame_index_.erase(deleteEvent.server_frame());
        }
    }
    
    // 6. 异步写入磁盘
    if(sink_)
    {
        sink_->Record(event, RecordToken());
    }
}


// ----- 查询接口（线程安全）----

// 按玩家 ID 查询最近 N 个事件
std::vector<GameEvent> EventStore::QueryByPlayer(uint32_t player_id, uint32_t limit /*= 20*/) const
{
    std::lock_guard<std::mutex> lk(mutex_);
    auto itr = player_index_.find(player_id);
    if(itr == player_index_.end())
    {
        return std::vector<GameEvent>();
    }

    int count = 0;
    std::vector<GameEvent> vecResultEvents;
    const auto& vecPlayerEvent = (itr->second);
    for(auto itr = vecPlayerEvent.rbegin(); itr != vecPlayerEvent.rend(); ++itr)
    {
        EventId eventId = (*itr);
        auto itr_find = events_.find(eventId);
        if(itr_find == events_.end())
        {
            continue;
        }

        vecResultEvents.push_back(itr_find->second);
        count += 1;
        if(limit > 0 && count >= limit)
        {
            break;
        }
    }

    return vecResultEvents;
}


// 按帧号范围查询事件([start_frame, end_frame])
std::vector<GameEvent> EventStore::QueryByFrameRange(uint32_t start_frame, uint32_t end_frame) const
{
    if(start_frame > end_frame)
    {
        return std::vector<GameEvent>();
    }

    std::lock_guard<std::mutex> lk(mutex_);
    std::vector<GameEvent> vecResultEvents;
    auto itr = frame_index_.lower_bound(start_frame);
    for( ; itr != frame_index_.end() && (itr->first) <= end_frame; ++itr)
    {
        const auto& vecFrameEvent = (itr->second);
        for(const auto& eventId : vecFrameEvent)
        {
            auto itr_find = events_.find(eventId);
            if(itr_find == events_.end())
            {
                continue;
            }

            vecResultEvents.push_back(itr_find->second);
        }
    }

    return vecResultEvents;
}


// 获取当前内存中的事件总数
size_t EventStore::GetEventCount() const
{
    std::lock_guard<std::mutex> lk(mutex_);
    return events_.size();
}
