#pragma once

#include <functional>
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>
#include <array>
#include "DirtyTracker.h"
#include "../GameServerMsgTypeDefine.h"
#include "../../../build/proto_gen/attribute_sync.pb.h"


class IAOIManager; // 前向声明，避免循环依赖


class DeltaSyncManager {
public:
    static constexpr int HISTORY_VERSION_SIZE = 64; // 当前保存最多的历史版本数量，超过这个数量的历史版本将被丢弃
    struct HistoryVersion {
        uint32_t current_version = 0;                            // 当前版本号
        //uint32_t baseVersionNo = 0;                                      // history 数组中下标 0 对应的版本号
        std::map<uint32_t, AttributeDelta> deltas;       // Key=版本号, Value=增量包, 自动升序, 使用std::map是为了方便按版本号排序和查找,同时比环形数组更加简单
        bool initialized = false;                                        // 是否已经初始化过


        // 压入新版本
        void Push(const AttributeDelta& delta) {
            uint32_t ver = delta.version_no();
            deltas[ver] = delta;                       // 存储或覆盖
            if (deltas.size() > HISTORY_VERSION_SIZE) {
                // 删除最小的 Key（即最旧的历史）
                deltas.erase(deltas.begin());
            }

            current_version = std::max(ver, current_version);
        }

        // 检查某版本是否还在历史窗口中
        bool Contains(uint32_t ver) const {
            return deltas.find(ver) != deltas.end();
        }

        // 获取最旧的版本号（用于判断是否过期）
        uint32_t GetOldestVersion() const {
            if (deltas.empty())
            {
                return 0;
            }
            
            
            return deltas.begin()->first;
        }


        // 获取从 from_ver 到当前最新版本的所有增量（返回已排序的列表）
        std::vector<AttributeDelta> GetRange(uint32_t from_ver) const {
            std::vector<AttributeDelta> result;
            auto it = deltas.lower_bound(from_ver);   // 找到 >= from_ver 的第一个元素
            while (it != deltas.end()) {
                result.push_back(it->second);
                ++it;
            }

            return result;
        }
    };

public:
    using SendCallback = std::function<void(uint32_t conn_id, const std::string& data, GameServerMsgType)>;

    DeltaSyncManager(std::shared_ptr<IAOIManager> aoiManager, SendCallback sendCallback) 
    : aoiManager_(aoiManager), sendCallback_(sendCallback) 
    {

    }
    
    ~DeltaSyncManager() = default;  

public:
    // 负责处理脏位的同步逻辑
    void Tick(uint32_t tickMs);

    // 因为调用层，当玩家属性变化发生时
    void OnAttributeChanged(int entityId, int attributeIndex, int64_t value);

    // 处理客户端的Nack的请求，客户端可能会因为网络丢包等原因没有收到某个版本的同步数据
    void OnNackRequest(uint32_t conn_id, int entityId, uint32_t fromVersion);

private:
    bool BuildFullSnapshot(uint32_t entityId, AttributeDelta& outDelta);
    void StoreHistory(uint32_t entityId, const AttributeDelta& delta);
    void SendDeltaToView(uint32_t entityId, AttributeDelta& delta);
    static uint32_t GetFieldPriority(uint8_t fieldId);

private:
    std::shared_ptr<IAOIManager> aoiManager_;
    SendCallback sendCallback_;

    

    struct EntitySyncInfo {
        DirtyTracker dirtyTracker;                          // 每个实体的脏位 
        std::unordered_map<uint8_t, int64_t>  field_cache_;    // 当前最新的属性数值
        HistoryVersion HistoryVersion_;                     // 历史版本信息        
    };

    std::unordered_map<int, std::shared_ptr<EntitySyncInfo>> entityAttributeMap_; // 属性索引 -> 属性数据
};