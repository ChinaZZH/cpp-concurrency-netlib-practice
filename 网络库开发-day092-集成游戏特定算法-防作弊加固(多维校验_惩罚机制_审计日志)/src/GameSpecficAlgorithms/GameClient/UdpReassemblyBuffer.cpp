#include "UdpReassemblyBuffer.h"

void UdpReassemblyBuffer::PushDelta(uint32_t entityId, const AttributeDelta& delta) 
{
    int version = delta.version_no();
    bool bFull = delta.is_full_snapshot();

    // 1. 全量快照：强制覆盖，更新版本记录
    if(bFull)
    {
        ApplyDelta(entityId, delta);
        entityBuffers_[entityId].expectedVersion = version + 1;
        entityBuffers_[entityId].pendingQueue_.clear();        
        std::cout << "[Client] Full snapshot applied, version=" << version << std::endl; 
    }
    else{
        
        auto& entityBuffer = entityBuffers_[entityId];
        if(version < entityBuffer.expectedVersion)
        {
            // 过期，丢弃, 说明这个数据包不是客户端所需要的
            return;
        }

        if(version == entityBuffer.expectedVersion)
        {
            ApplyDelta(entityId, delta);
            entityBuffer.expectedVersion++;
            // 检查是否有后续的增量包可以应用
            for(auto itr = entityBuffer.pendingQueue_.begin(); itr != entityBuffer.pendingQueue_.end(); )
            {
                if(itr->first != entityBuffer.expectedVersion)
                {
                    break;
                }
                
                ApplyDelta(entityId, itr->second);
                entityBuffer.expectedVersion++; 
                itr = entityBuffer.pendingQueue_.erase(itr);
            }
        }
        else{
            // 版本号大于期待的版本号，说明有丢包，先缓存起来
            entityBuffer.pendingQueue_[version] = delta;
            CheckGapAndNACK(entityId, version);
        }
    }
}

   

void UdpReassemblyBuffer::ApplyDelta(uint32_t entity_id, const AttributeDelta& delta)
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


void UdpReassemblyBuffer::CheckGapAndNACK(uint32_t entity_id, uint32_t received_version)
{
    // 收到的消息版本号小于期待的版本号，说明有丢包，触发NACK
    auto now = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
    auto& entityBuffer = entityBuffers_[entity_id];
    if(received_version > entityBuffer.expectedVersion && now - entityBuffer.lastNackTime > 200) // 200ms间隔触发NACK
    {
        nackCallback_(entity_id, entityBuffer.expectedVersion);
        entityBuffer.lastNackTime = now;
    }   
}
