#pragma once
#include <functional>
#include <string>
#include <atomic>
#include "InputBuffer.h"

class FrameScheduler {
public:
    using BroadcastCallback = std::function<void(const std::string& serialized_pkg)>;

    FrameScheduler(InputBuffer* inputBuffer, BroadcastCallback broadcastCallback)
        : buffer_(inputBuffer), current_frame_index_(1), broadcastCallback_(broadcastCallback) 
        {

        }

    // 启动帧调度器
    void OnTick();

    uint32_t GetServerFrameIndex() { return current_frame_index_; }

private:
    uint32_t AddServerFrameIndex();

private:
    InputBuffer* buffer_;
    std::atomic<uint32_t> current_frame_index_;
    BroadcastCallback broadcastCallback_;
};