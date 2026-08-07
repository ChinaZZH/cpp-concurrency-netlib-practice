#include "MatchManager.h"
#include <iostream>
#include <sstream>

MatchManager::MatchManager() 
{

    last_match_attempt_ms_ = std::chrono::steady_clock::now();
}

// 初始化：设置超时时间（毫秒） 
void MatchManager::Init(uint32_t timeout_ms /*= 30000*/)
{
    timeout_ms_ = timeout_ms;
}

// 加入匹配队列(若已存在则更新 mmr 并重置计时)
void MatchManager::JoinQueue(uint32_t player_id, uint32_t mmr)
{
    MatchCandidate& candidata = pool_[player_id];
    candidata.player_id = player_id;
    candidata.mmr = mmr;
    candidata.enqueue_time_ms =  std::chrono::steady_clock::now();
}

// 取消匹配
void MatchManager::LeaveQueue(uint32_t player_id)
{
    this->RemovePlayer(player_id);
}


// 定期tick (由上层定时器调用，如每秒一次）
void MatchManager::Tick()
{
    std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    std::vector<uint32_t> timeoutPlayers;
    for(auto& [player_id, candidate] : pool_)
    {
        auto keep_mill_seconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - candidate.enqueue_time_ms).count();
        if(keep_mill_seconds < timeout_ms_)
        {
            continue;
        }

        timeoutPlayers.push_back(player_id);
    }

    for(auto& player_id : timeoutPlayers)
    {
        this->RemovePlayer(player_id);
        if(timeout_cb_)
        {
            timeout_cb_(player_id);
        }
    }

    // 每 2 秒尝试一次配对（避免频繁遍历）
    auto match_mill_seconds = std::chrono::duration_cast<std::chrono::milliseconds>(now - last_match_attempt_ms_).count();
    if(match_mill_seconds >= 2000)
    {
        TryMatch();
        last_match_attempt_ms_ = now;
    }
}




void MatchManager::RemovePlayer(uint32_t player_id)
{
    pool_.erase(player_id);
}


 // 设置配对参数
void MatchManager::SetMatchConfig(uint32_t match_size, uint32_t max_mmr_grap)
{
    match_size_ = match_size;
    max_mmr_grap_ = max_mmr_grap;

    std::cout << "[Match] Config set: match_size=" << match_size_;
    std::cout << ", max_mmr_gap=" << max_mmr_grap_ << std::endl;
}


// 手动触发配对（由上层调用或 Tick 触发）
void MatchManager::TryMatch()
{
    // 人数不足，无法配对
    if(pool_.size() < match_size_)
    {
        return ;
    }


    // 1. 收集所有候选并按 MMR 排序
    std::vector<MatchCandidate> candidate;
    candidate.reserve(pool_.size());
    for(const auto& kv : pool_)
    {
        candidate.push_back(kv.second);
    }

    std::sort(candidate.begin(), candidate.end(), [](const MatchCandidate& pre, const MatchCandidate& next){
        return pre.mmr <= next.mmr;
    });

    // 2. 滑动窗口配对
    size_t i = 0 ;
    while(i + match_size_ <= candidate.size())
    {
        uint32_t lowest_mmr = candidate[i].mmr;
        uint32_t highest_mmr = candidate[i + match_size_ - 1].mmr;
        if(highest_mmr - lowest_mmr <= max_mmr_grap_)
        {
            // 配对成功：取出 match_size_ 个玩家
            std::vector<uint32_t> matched_players;
            for(size_t j = 0; j < match_size_; ++j)
            {
                matched_players.push_back(candidate[i + j].player_id);

                // 从池中移除
                pool_.erase(candidate[i + j].player_id);
            }

            // 通知上层
            if(match_success_cb_)
            {
                match_success_cb_(matched_players);
            }

            std::cout << "[Match] Match success! Players: ";
            for (uint32_t pid : matched_players) {
                std::cout << pid << " ";
            }
            std::cout << std::endl;

            // 继续从下一个位置开始匹配
            i += match_size_;
        }
        else
        {
            // 分差过大，跳过当前最低分玩家
            i += 1;
        }
        
    } 
}
