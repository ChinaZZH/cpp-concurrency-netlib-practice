#pragma once


enum GameServerMsgType
{
    GSMT_Error              = 0,
    // AOI算法 + 位置同步
    GSMT_AddEntity          = 1,
    GSMT_RemoveEntity       = 2,
    GSMT_MoveEntity         = 3,
    GSMT_SyncNeighborsEntity         = 4,

    // 状态同步， 属性同步
    GSMT_SyncAttributeDelta = 11,
    GSMT_NACK_REQUEST       = 12,

    // 帧同步
    GSMT_FrameClientInput   = 21,
    GSMT_FrameServerPackage = 22,
};