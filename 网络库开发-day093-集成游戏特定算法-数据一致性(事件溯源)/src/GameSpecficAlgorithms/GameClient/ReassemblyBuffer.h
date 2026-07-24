#pragma once

#include <functional>
#include <map>
#include <unordered_map>
#include "../../../build/proto_gen/attribute_sync.pb.h"

class ReassemblyBuffer {
public:
    void PushDelta(uint32_t entityId, const AttributeDelta& delta);

private:
    void ApplyDelta(uint32_t entity_id, const AttributeDelta& delta);
    
private:
   std::unordered_map<uint32_t, uint32_t> last_applied_version_; // 记录已应用的最大版本号
};