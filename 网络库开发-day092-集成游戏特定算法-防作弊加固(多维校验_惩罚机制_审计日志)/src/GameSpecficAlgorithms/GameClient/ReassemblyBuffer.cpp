#include "ReassemblyBuffer.h"

void ReassemblyBuffer::PushDelta(uint32_t entityId, const AttributeDelta& delta) 
{
    int version = delta.version_no();
    bool bFull = delta.is_full_snapshot();

    // 1. 全量快照：强制覆盖，更新版本记录
    if(bFull)
    {
        ApplyDelta(entityId, delta);
        last_applied_version_[entityId] = version;
        std::cout << "[Client] Full snapshot applied, version=" << version << std::endl; 
    }else{
         // 2. 普通增量：检查是否重复（TCP 重传可能导致重复包）
        auto it = last_applied_version_.find(entityId);
        if (it != last_applied_version_.end() && version <= it->second) {
            // 已经应用过更新的或相同的版本，丢弃
            return;
        }

        // 3. TCP 保证顺序，version 一定等于 last_applied_version_ + 1（正常情况下）
        //    直接应用，无需校验连续性
        ApplyDelta(entityId, delta);
        last_applied_version_[entityId] = version;
    }
}

   

void ReassemblyBuffer::ApplyDelta(uint32_t entity_id, const AttributeDelta& delta)
{
    // 此处应将 delta 中的字段应用到游戏对象
    // 示例：存入外部缓存或直接修改对象属性
    for (const auto& field : delta.fields()) 
    {
        // field.field_id(), field.value()
        // 应用到游戏对象
        std::cout << "[Client] Entity " << entity_id 
                  << " field " << field.field_id() 
                  << " updated." << std::endl;
    }
}


