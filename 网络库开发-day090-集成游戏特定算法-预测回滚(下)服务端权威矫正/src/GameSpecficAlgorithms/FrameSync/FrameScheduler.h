#pragma once
#include <functional>
#include <string>
#include <atomic>
#include "InputBuffer.h"
#include "ServerPlayerManager.h"

class FrameScheduler {
public:
    using BroadcastCallback = std::function<void(const std::string& serialized_pkg)>;

    using CorrectionCallback = std::function<void(uint32_t playerId, const CorrectionEntry& connectionEntry)>;

    FrameScheduler(InputBuffer* inputBuffer, BroadcastCallback broadcastCallback, CorrectionCallback correctionCallback)
        : buffer_(inputBuffer)
        , broadcastCallback_(broadcastCallback)
        , correction_cb_(correctionCallback) 
        , server_player_mgr_(std::make_shared<ServerPlayerManager>())
        , server_frame_index_(1)
        {

        }


    // ===== 由 20ms 定时器调用 =====
    void OnLogicTick();

    // 启动帧调度器
    void OnBroadcastTick();

    uint32_t GetServerFrameIndex() { return server_frame_index_; }

    std::shared_ptr<ServerPlayerManager> GetServerPlayerManager() { return server_player_mgr_; }

private:
    uint32_t AddServerFrameIndex();

private:
    InputBuffer* buffer_;
    BroadcastCallback broadcastCallback_;
    CorrectionCallback correction_cb_;

    std::shared_ptr<ServerPlayerManager> server_player_mgr_;
    std::atomic<uint32_t> server_frame_index_;
};