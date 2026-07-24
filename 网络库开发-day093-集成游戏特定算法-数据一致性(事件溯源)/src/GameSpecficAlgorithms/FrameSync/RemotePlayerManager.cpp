#include "RemotePlayerManager.h"

// 1. 更新远程玩家的状态（由网络层收到广播时调用）
//    timestamp_ms: 服务端生成该状态的时间（毫秒）
void RemotePlayerManager::UpdateRemoteState(uint32_t player_id, Fixed x, Fixed y, uint32_t server_frame)
{
    RemoteStateSnapshot snapshot;
    snapshot.x = x;
    snapshot.y = y;
    snapshot.valid = true;
    snapshot.server_frame = server_frame;

   RemotePlayerSmoother& remoteSmoother = smoothers_[player_id];
   remoteSmoother.PushState(snapshot);
}

// 2. 获取远程玩家当前应渲染的位置（由渲染循环调用）
//    返回值：true 表示有有效插值结果，out_x/out_y 被填充
//            false 表示无有效数据，调用方应使用兜底值或跳过渲染
bool RemotePlayerManager::GetRenderPosition(uint32_t player_id, uint32_t render_frame, Fixed& out_x, Fixed& out_y) const
{
    auto itr = smoothers_.find(player_id);
    if(itr == smoothers_.end())
    {
        return false;
    }

    const RemotePlayerSmoother& smoother = (itr->second);
    if(false == smoother.IsReady())
    {
        return false;
    }

    RemoteStateSnapshot snapShot = smoother.GetRenderState(render_frame);
    if(false == snapShot.valid)
    {
        return false;
    }

    out_x = snapShot.x;
    out_y = snapShot.y;
    return true;
}

// 3. 移除玩家（玩家退出时调用）
void RemotePlayerManager::RemovePlayer(uint32_t player_id)
{
    smoothers_.erase(player_id);
}

// 4. 清空所有（断线重连时调用）
void RemotePlayerManager::Clear()
{
    smoothers_.clear();
}