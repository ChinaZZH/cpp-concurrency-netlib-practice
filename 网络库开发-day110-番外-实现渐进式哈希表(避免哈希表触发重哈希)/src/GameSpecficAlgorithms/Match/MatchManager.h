#pragma once

// MatchManager.h
#include <unordered_map>
#include <functional>
#include <cstdint>
#include <chrono>

struct MatchCandidate
{
    uint32_t    player_id;
    uint32_t    mmr;
    std::chrono::steady_clock::time_point  enqueue_time_ms;
};


class MatchManager
{
public:
    using TimeoutCallback = std::function<void(uint32_t player_id)>;

    using MatchSuccessCallback = std::function<void(const std::vector<uint32_t>& vecPlayerIdList)>;

public:
    MatchManager();

    // 初始化：设置超时时间（毫秒） 
    void Init(uint32_t timeout_ms = 30000);

    // 加入匹配队列(若已存在则更新 mmr 并重置计时)
    void JoinQueue(uint32_t player_id, uint32_t mmr);

    // 取消匹配
    void LeaveQueue(uint32_t player_id);

    // 定期tick (由上层定时器调用，如每秒一次）
    void Tick();

    // 获取队列大小
    size_t GetPoolSize() const { return pool_.size(); }

    // 设置回调
    void SetTimeoutCallback(TimeoutCallback cb)             { timeout_cb_ = cb; }

    void SetMatchSuccessCallback(MatchSuccessCallback cb)   { match_success_cb_ = cb; }

    // 设置配对参数
    void SetMatchConfig(uint32_t match_size, uint32_t max_mmr_grap);

    // 手动触发配对（由上层调用或 Tick 触发）
    void TryMatch();

private:
    void RemovePlayer(uint32_t player_id);

private:
    std::unordered_map<uint32_t, MatchCandidate> pool_;
    uint32_t timeout_ms_ = 30000;
    uint64_t last_tick_ms_ = 0;

    TimeoutCallback timeout_cb_;
    MatchSuccessCallback match_success_cb_;

    // 配对配置
    uint32_t match_size_        = 2;        // 每局人数
    uint32_t max_mmr_grap_      = 100;      // 允许最大分差

    // 上次配对尝试时间(用于 Tick 中的限频)
    std::chrono::steady_clock::time_point last_match_attempt_ms_;
};