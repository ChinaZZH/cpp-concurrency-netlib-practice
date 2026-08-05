#pragma once

#include <functional>
#include <map>
#include <unordered_map>
#include "../../../build/proto_gen/attribute_sync.pb.h"

class UdpReassemblyBuffer {
public:
    using NACKCallBack = std::function<void(uint32_t entityId, uint32_t fromVersion)>;
    explicit UdpReassemblyBuffer(NACKCallBack nackCallback)
        : nackCallback_(nackCallback) {}
    

    void PushDelta(uint32_t entityId, const AttributeDelta& delta);

private:
    void ApplyDelta(uint32_t entity_id, const AttributeDelta& delta);
    void CheckGapAndNACK(uint32_t entity_id, uint32_t received_version);

private:
    NACKCallBack nackCallback_;

    struct EntityBuffer {
        std::map<uint32_t, AttributeDelta> pendingQueue_;   // 按版本号排序的积压库存
        uint32_t expectedVersion = 0;                       // 期待的版本号
        uint64_t lastNackTime = 0;                          // 上次收到增量包的时间戳
    };

    std::unordered_map<uint32_t, EntityBuffer> entityBuffers_; // 每个实体的缓冲区    
};