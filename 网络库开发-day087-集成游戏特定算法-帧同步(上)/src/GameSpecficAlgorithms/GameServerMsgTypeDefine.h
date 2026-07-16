#pragma once


enum GameServerMsgType
{
    GSMT_Error              = 0,
    GSMT_AddEntity          = 1,
    GSMT_RemoveEntity       = 2,
    GSMT_MoveEntity         = 3,
    GSMT_SyncNeighborsEntity         = 4,

    GSMT_SyncAttributeDelta = 11,
    GSMT_NACK_REQUEST       = 12,
};