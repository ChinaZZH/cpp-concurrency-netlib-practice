// EventSink.h
#pragma once
#include <atomic>
#include <thread>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <fstream>
#include <string>
#include "../../../build/proto_gen/game_event.pb.h"


// 事件接收器：负责异步写入事件到文件
class EventStore;
// 记录一个事件（线程安全，非阻塞）
struct RecordToken
{
private:
    RecordToken() = default;
    friend class EventStore;
};

class EventSink {
public:
    EventSink();
    ~EventSink();

    // 初始化：指定存储路径，启动后台写入线程
    void Init(const std::string& file_path);

    

    void Record(const GameEvent& event, RecordToken token);

    // 关闭并等待所有事件写入完成
    void Shutdown();

private:
    // 后台写入线程的主循环
    void WriteLoop();

    void SerializeGameEventToFile(const GameEvent& event);

private:
    std::string file_path_;
    std::ofstream output_file_;

    // 生产者-消费者队列
    std::queue<GameEvent> pending_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;

    std::thread writer_thread_;
    std::atomic<bool> running_{false};
    std::atomic<bool> shutdown_requested_{false};
};