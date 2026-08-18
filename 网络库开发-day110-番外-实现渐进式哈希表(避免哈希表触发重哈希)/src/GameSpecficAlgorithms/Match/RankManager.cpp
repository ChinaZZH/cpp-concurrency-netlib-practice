#include "RankManager.h"
#include <iostream>
#include <sstream>

RankManager::RankManager()
{

}

// 初始化：设置分片大小（默认 100 分/片）
void RankManager::Init(uint32_t shard_size /*= 100*/)
{
    shard_size_ = shard_size;
    std::cout << "[Rank] Initialized with shard_size=" << shard_size_ << std::endl;
}


// 更新玩家分数（如果玩家不存在则新增）
void RankManager::UpdateScore(uint32_t player_id, int32_t new_mmr)
{
    uint32_t newShardId = this->GetShardId(new_mmr);

    auto itr_player = player_index_.find(player_id);
    if(itr_player == player_index_.end())
    {
        PlayerRankInfo playerInfo;
        playerInfo.player_id = player_id;
        playerInfo.mmr = new_mmr;
        playerInfo.shard_id = newShardId;
        player_index_.insert(std::make_pair(player_id, playerInfo));

        this->AddToShard(player_id, new_mmr);
    }
    else
    {
        // mmr分数并未改变
        PlayerRankInfo& playerInfo = (itr_player->second);
        if(playerInfo.mmr == new_mmr)
        {
            return;
        }

        // 判断是否在同一个shard_id里面
        this->RemoveFromShard(playerInfo);

        playerInfo.mmr = new_mmr;
        playerInfo.shard_id = newShardId;
        this->AddToShard(player_id, new_mmr);
    }
}


// 获取 Top N 玩家
std::vector<uint32_t> RankManager::GetTopN(uint32_t n) const
{
    if(n <= 0)
    {
        return std::vector<uint32_t>();
    }

  
    std::vector<uint32_t> vecTopN;
    vecTopN.reserve(n);
    for(auto itr = shards_.rbegin(); itr != shards_.rend(); ++itr)
    {
        const RankShard& rank = (itr->second);
        for(auto itr_player = rank.players_.begin(); itr_player != rank.players_.end(); ++itr_player)
        {
            vecTopN.emplace_back(itr_player->first);
            
            // 结束循环
            if(vecTopN.size() >= n)
            {
                return vecTopN;
            }
        }
    }

    return vecTopN;
}

// 获取玩家排名(从1开始，-1表示不存在)
int32_t RankManager::GetRank(uint32_t player_id) const
{
    auto itr_player = player_index_.find(player_id);
    if(itr_player == player_index_.end())
    {
        return -1;
    }

    int playerCount = 0;
    const PlayerRankInfo& playerInfo = (itr_player->second);
    for(auto itr = shards_.rbegin(); itr != shards_.rend(); ++itr)
    {
        if((itr->first) <= (playerInfo.shard_id))
        {
            break;
        }

        playerCount += (itr->second).players_.size();
    }


    int rankNo = playerCount;
    auto itr_shard = shards_.find(playerInfo.shard_id);
    if(itr_shard != shards_.end())
    {
        const RankShard& rank = (itr_shard->second);
        auto itr_top = std::find_if(rank.players_.begin(),  rank.players_.end(), [player_id](const std::pair<uint32_t, uint32_t>& playerData){
            return playerData.first == player_id;
        });

        if(itr_top != rank.players_.end())
        {
            int index = itr_top - rank.players_.begin();
            rankNo += index;
        }
    }

    rankNo += 1;
    return rankNo;
}


// 获取玩家当前分数
bool RankManager::GetScore(uint32_t player_id, uint32_t& out_mmr) const
{
    auto itr = player_index_.find(player_id);
    if(itr == player_index_.end())
    {
        return false;
    }

    const PlayerRankInfo& rankData = (itr->second);
    out_mmr = rankData.mmr;
    return true;
}

// 获取总玩家数
size_t RankManager::GetTotalPlayers() const
{
    return player_index_.size();
}

// 获取分片数量
size_t RankManager::GetShardCount() const
{
    return shards_.size();
}


// 调试：打印分片分布
void RankManager::PrintShards() const
{
    std::cout << "[Rank] Shards: " << shards_.size() << ", Total players: " << player_index_.size() << std::endl;
    for (const auto& kv : shards_) {
        std::cout << "  Shard " << kv.first << " [" << kv.second.min_score << "-" << kv.second.max_score;
        std::cout << "]: " << kv.second.players_.size() << " players" << std::endl;

        for(const auto& playerInfo : kv.second.players_)
        {
            std::cout << "   Player player_id:" << playerInfo.first << " score:" << playerInfo.second << std::endl;
        }
    }
}


// 内部方法
uint32_t RankManager::GetShardId(uint32_t mmr) const
{
    return mmr / shard_size_;
}

void RankManager::AddToShard(uint32_t player_id, uint32_t mmr)
{
    uint32_t shard_index = this->GetShardId(mmr);
    auto itr = shards_.find(shard_index);
    if(itr == shards_.end())
    {
        RankShard rank;
        rank.shard_id = shard_index;
        rank.min_score = (shard_index * shard_size_);
        rank.max_score = (rank.min_score + shard_size_ - 1);
        rank.players_.push_back(std::pair(player_id, mmr));

        shards_.insert(std::make_pair(shard_index, rank));
    }
    else{
        RankShard& rank = (itr->second);
        InsertIntoSortedList(rank.players_, player_id, mmr);
    }
}


void RankManager::RemoveFromShard(PlayerRankInfo& rankInfo)
{
    auto itr_rank = shards_.find(rankInfo.shard_id);
    if(itr_rank != shards_.end())
    {
        uint32_t player_id = rankInfo.player_id;
        RankShard& shardRank = (itr_rank->second);
        std::erase_if(shardRank.players_, [player_id](const std::pair<uint32_t, uint32_t>& pairData){
            return pairData.first == player_id;
        });

        if(shardRank.players_.empty())
        {
            shards_.erase(rankInfo.shard_id);
        }
    }
}


void RankManager::InsertIntoSortedList(RankShard::VecRankShardPlayers& vecSortPlayer, uint32_t player_id, uint32_t mmr)
{
    auto itr = std::find_if(vecSortPlayer.begin(), vecSortPlayer.end(), [mmr](const std::pair<uint32_t, uint32_t>& pairData){
            return pairData.second < mmr;
        });


    auto playerData = std::pair(player_id, mmr);
    if(itr == vecSortPlayer.end())
    {
        vecSortPlayer.push_back(playerData);
    }
    else
    {
        vecSortPlayer.insert(itr, playerData);
    }
}