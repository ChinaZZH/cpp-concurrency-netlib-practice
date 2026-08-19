#pragma once

#include <unordered_map>
#include <functional>
#include <cstdint>
#include <chrono>
#include <map>
#include <list>
#include <vector>

struct PlayerRankInfo 
{
    uint32_t    player_id;
    uint32_t    mmr;
    uint32_t    shard_id;
};


struct RankShard
{
    uint32_t    shard_id;
    uint32_t    min_score;
    uint32_t    max_score;

    using VecRankShardPlayers = std::vector<std::pair<uint32_t, uint32_t>>;
    VecRankShardPlayers players_; // 按 MMR 降序排列的 player_id 列表
};


class RankManager
{
public:
    RankManager();

    // 初始化：设置分片大小（默认 100 分/片）
    void Init(uint32_t shard_size = 100);

    // 更新玩家分数（如果玩家不存在则新增）
    void UpdateScore(uint32_t player_id, int32_t new_mmr);

    // 获取 Top N 玩家
    std::vector<uint32_t> GetTopN(uint32_t n) const;

    // 获取玩家排名(从1开始，-1表示不存在)
    int32_t GetRank(uint32_t player_id) const;

    // 获取玩家当前分数
    bool GetScore(uint32_t player_id, uint32_t& out_mmr) const;

    // 获取总玩家数
    size_t GetTotalPlayers() const;

    // 获取分片数量
    size_t GetShardCount() const;

     // 调试：打印分片分布
     void PrintShards() const;

private:
    // 内部方法
    uint32_t GetShardId(uint32_t mmr) const;

    void AddToShard(uint32_t player_id, uint32_t mmr);

    void RemoveFromShard(PlayerRankInfo& rankInfo);

    void InsertIntoSortedList(RankShard::VecRankShardPlayers& sortPlayer, uint32_t player_id, uint32_t mmr);

private:
    // 配置
    uint32_t shard_size_   = 100;

    // 分片存储：shard_id → RankShard
    std::map<uint32_t, RankShard> shards_;

    // 玩家索引：player_id → PlayerRankInfo
    std::unordered_map<uint32_t, PlayerRankInfo> player_index_;
};