#pragma once
#include <string>
#include <cstdint>
#include <fstream>
#include <mutex>
#include <queue>
#include <thread>
#include <atomic>
#include <chrono>
#include <condition_variable>

struct AuditEntry
{
    uint64_t timestamp_ms;
    uint32_t player_id;
    std::string     violation_type;      // "speed", "attack_freq", "frame_skip"
    std::string     current_value;       // 如距离值、帧号差值等
    std::string     threshold_value;     // 对应的阈值
    uint32_t violation_count;            // 当前累计违规次数
    std::string     penalty_action;      // "none", "correction", "kick", "ban"
};


class AuditLogger
{
public:
    AuditLogger();
    ~AuditLogger();

    // 初始化（指定日志文件路径）
    void Init(const std::string& log_path, bool async_mode = true);

    // 记录一条审计日志（线程安全）
    void Log(const AuditEntry& entry);

    // 关闭日志系统（等待队列中的日志写完）
    void Shutdown();

private:
    // 异步写入线程的主循环
    void FlushWorker();

    void WriteEntry(const AuditEntry& entry);

private:
    std::string log_path_;
    std::ofstream log_file_;
    std::queue<AuditEntry> pending_queue_;
    std::mutex queue_mutex_;
    std::condition_variable queue_cv_;
    std::thread worker_thread_;
    std::atomic<bool> running_ = false;
    bool async_mode_ = true; 
};