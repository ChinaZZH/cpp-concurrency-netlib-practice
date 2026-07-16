#include "DeltaSyncManager.h"
#include "../AOI/IAOIManager.h"




// 因为调用层，当玩家属性变化发生时
void DeltaSyncManager::OnAttributeChanged(int entityId, int attributeIndex, int64_t value)
{
    std::shared_ptr<EntitySyncInfo> ptrSync;
    auto itr = entityAttributeMap_.find(entityId);
    if(itr == entityAttributeMap_.end())
    {
        ptrSync = std::make_shared<EntitySyncInfo>();
        ptrSync->HistoryVersion_.initialized = true;
        ptrSync->HistoryVersion_.current_version = 0;
        //ptrSync->HistoryVersion_.baseVersionNo = 0;

        entityAttributeMap_[entityId] = ptrSync;
    }
    else{
        ptrSync = (itr->second);
    }

    ptrSync->dirtyTracker.MarkDirty(attributeIndex);
    ptrSync->field_cache_[attributeIndex] = value;
}

// ----- 核心：全量快照构建（供 NACK 过期和降级使用）-----
bool DeltaSyncManager::BuildFullSnapshot(uint32_t entityId, AttributeDelta& outDelta)
{
    outDelta.set_entity_id(entityId);
    outDelta.set_is_full_snapshot(true);

    auto itr = entityAttributeMap_.find(entityId);
    if(itr == entityAttributeMap_.end())
    {
        return false;
    }

    auto syncDelta = (itr->second);
    outDelta.set_version_no(syncDelta->HistoryVersion_.current_version);
    for(auto& [fieldId, value] : syncDelta->field_cache_)
    {
       FieldUpdate* fieldUpdate = outDelta.add_fields();
       if (!fieldUpdate)
       {
            return false;
       }
       
       fieldUpdate->set_field_id(fieldId);
       fieldUpdate->set_value(value);
    }

    return true;
}

// ----- 核心：环形缓冲区存储 -----
void DeltaSyncManager::StoreHistory(uint32_t entityId, const AttributeDelta& delta)
{
    std::shared_ptr<EntitySyncInfo> ptrSync;
    auto itr = entityAttributeMap_.find(entityId);
    if(itr == entityAttributeMap_.end())
    {
        ptrSync = std::make_shared<EntitySyncInfo>();
        ptrSync->HistoryVersion_.initialized = true;
        ptrSync->HistoryVersion_.current_version = 0;

        entityAttributeMap_[entityId] = ptrSync;
    }
    else{
        ptrSync = (itr->second);
    }

    ptrSync->HistoryVersion_.Push(delta);
}

void DeltaSyncManager::SendDeltaToView(uint32_t entityId, AttributeDelta& delta)
{
    std::vector<int> vecNegibors = aoiManager_->GetNeighbors(entityId);
    if(vecNegibors.empty())
    {
        return;
    }   

    // mtu包的大小限制，暂时不做分包处理，直接丢弃(后续等一个全量包的大小大于这个数值再做分包处理)
    if(delta.ByteSizeLong() > 1400)
    {
        std::cout << "DeltaSyncManager::SendDeltaToView: delta.ByteSizeLong() > 1400, entityId=" << entityId << ", version=" << delta.version_no() << std::endl;
        return;
    }

    for(auto& neighborConId : vecNegibors)
    {
        delta.set_msg_client_id(neighborConId);
        sendCallback_(neighborConId, delta.SerializeAsString(), GameServerMsgType::GSMT_SyncAttributeDelta);
    }
}

uint32_t DeltaSyncManager::GetFieldPriority(uint8_t fieldId)
{
    switch (fieldId)
    {
    case 1:
        return 100; // HP
    case 2:
        return 90;  // MP
    
    default:
        return 10;
    }

    return 10;
}


// 处理客户端的Nack的请求，客户端可能会因为网络丢包等原因没有收到某个版本的同步数据
void DeltaSyncManager::OnNackRequest(uint32_t conn_id, int entityId, uint32_t fromVersion)
{
    auto itr = entityAttributeMap_.find(entityId);
    if(itr == entityAttributeMap_.end())
    {
        return;     
    }

    auto syncDelta = (itr->second);
    if(false == syncDelta->HistoryVersion_.Contains(fromVersion))
    {
        // 如果不包含，则说明客户端请求的版本已经过期了，需要发送全量           
        AttributeDelta fullSnapshot;
        fullSnapshot.set_msg_client_id(conn_id);
        fullSnapshot.set_is_full_snapshot(true);
        fullSnapshot.set_entity_id(entityId);
        fullSnapshot.set_version_no(syncDelta->HistoryVersion_.current_version);
        if(BuildFullSnapshot(entityId, fullSnapshot))
        {
            sendCallback_(conn_id, fullSnapshot.SerializeAsString(), GameServerMsgType::GSMT_SyncAttributeDelta);
        }

        return;
    }

    // 如果包含，则说明客户端请求的版本还在历史窗口中，直接发送增量包
    std::unordered_map<int32_t, int64_t> syncField;
    std::vector<AttributeDelta> deltas = syncDelta->HistoryVersion_.GetRange(fromVersion);
    for(auto& delta : deltas)
    {
       for(int i = 0; i < delta.fields_size(); ++i)
        {
            const FieldUpdate& fieldUpdate = delta.fields(i);
            syncField[fieldUpdate.field_id()] = fieldUpdate.value();
        }
    }

    // 构建一个新的增量包，包含所有需要同步的字段
    AttributeDelta newDelta;
    newDelta.set_msg_client_id(conn_id);
    newDelta.set_is_full_snapshot(false);
    newDelta.set_entity_id(entityId);
    newDelta.set_version_no(syncDelta->HistoryVersion_.current_version);
    for(auto& [fieldId, value] : syncField)
    {
       FieldUpdate* fieldUpdate = newDelta.add_fields();
       if (!fieldUpdate)
       {
            continue;   
       }
       
       fieldUpdate->set_field_id(fieldId);
       fieldUpdate->set_value(value);
    }

    sendCallback_(conn_id, newDelta.SerializeAsString(), GameServerMsgType::GSMT_SyncAttributeDelta);
}       


// 负责处理脏位的同步逻辑
void DeltaSyncManager::Tick(uint32_t tickMs)
{
    for(auto& [entityId, ptrSync] : entityAttributeMap_)
    {
        if(false == ptrSync->dirtyTracker.HasAnyDirty())
        {
            continue;
        }

        uint64_t dirtyBits = ptrSync->dirtyTracker.GetDirtyBits();
        ptrSync->dirtyTracker.ClearAll();

        AttributeDelta delta;
        delta.set_entity_id(entityId);
        delta.set_is_full_snapshot(false);
        delta.set_version_no(ptrSync->HistoryVersion_.current_version + 1);

        for(int i = 0; i < sizeof(dirtyBits) * 8; ++i)
        {
            if(dirtyBits & (1ULL << i))
            {
                FieldUpdate* fieldUpdate = delta.add_fields();
                if (!fieldUpdate)
                {           
                    continue;   
                }   

                fieldUpdate->set_field_id(i);
                fieldUpdate->set_value(ptrSync->field_cache_[i]);
            }
        }

        // 存储历史版本
        ptrSync->HistoryVersion_.Push(delta);
        
        // 发送增量包给所有邻居
        SendDeltaToView(entityId, delta);
    }
}
   