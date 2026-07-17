#pragma once
#include <functional>
#include <string>
#include <atomic>
#include "InputBuffer.h"

class FrameScheduler {
public:
    using BroadcastCallback = std::function<void(const std::string& serialized_pkg)>;

    FrameScheduler(InputBuffer* inputBuffer, uint32_t interval_ms, BroadcastCallback broadcastCallback)
        : buffer_(inputBuffer), current_frame_index_(1), interval_ms_(interval_ms), broadcastCallback_(broadcastCallback) 
        {

        }

    // 启动帧调度器
    void OnTick();

    uint32_t GetServerFrameIndex() { return current_frame_index_; }

    uint32_t GetIntervalMs() { return interval_ms_; }

private:
    uint32_t AddServerFrameIndex();

private:
    InputBuffer* buffer_;
    std::atomic<uint32_t> current_frame_index_;

    uint32_t interval_ms_ = 50; // 每帧间隔时间，单位毫秒
    BroadcastCallback broadcastCallback_;
};