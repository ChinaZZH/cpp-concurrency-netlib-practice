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
    GSMT_FrameSyncAckPackage = 23,
    GSMT_FrameSyncAddPlayer  = 24,
    GSMT_FrameSyncRemovePlayer = 25,

    GSMT_ServerCorrection    = 26,
    GSMT_FrameReconnect      = 27,   
    GSMT_FrameAttackRequest  = 28,
    GSMT_FrameHitResult      = 29,

    GSMT_RemovePlayerFromMatch = 41,
    GSMT_MatchSuccessdNotify   = 42,

    // 动态分区
    GSMT_MigrationRequest       = 51, // 客户端命令到服务端
    GSMT_MigrationData          = 52, // 服务端命令从source服务端结点 到 target服务器结点
    GSMT_MigrationAck           = 53, // target服务器结点回复迁移结果确认消息给source结点结点   
};