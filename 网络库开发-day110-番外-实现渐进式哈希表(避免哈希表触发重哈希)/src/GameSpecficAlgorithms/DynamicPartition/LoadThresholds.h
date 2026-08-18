#pragma once

#include <cstdint>

struct LoadThresholds
{
    uint32_t    max_players_per_partition  = 50;         // 分区最大玩家数
    uint32_t    max_entities_per_partition = 200;        // 分区最大实体数
    uint32_t    min_players_for_merge = 5;               // 低于此值考虑合并  
    float       check_interval_seconds = 5.0f;           // 检查间隔（秒）
    uint32_t    consecutive_overloads = 3;               // 连续超载次数才触发迁移

    static LoadThresholds Default() { 
        LoadThresholds t;
        t.max_players_per_partition  = 50;         // 分区最大玩家数
        t.max_entities_per_partition = 200;        // 分区最大实体数
        t.min_players_for_merge = 5;               // 低于此值考虑合并  
        t.check_interval_seconds = 5.0f;           // 检查间隔（秒）
        t.consecutive_overloads = 3;                // 连续超载次数才触发迁移
        return t; 
    }
};