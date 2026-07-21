#pragma once
#include <unordered_map>
#include <shared_mutex>
#include <vector>
#include "../../../build/proto_gen/frame_sync.pb.h"

class InputBuffer {
public:
    // 由 TcpServer OnMessage 调用（多线程安全）
    void PushInput(uint32_t playerId, uint32_t server_frame, const ClientInput& inputData);

    // 由帧调度器调用（单线程），取出指定帧的所有指令
    std::vector<ClientInput> FetchFrame(uint32_t frame_index);

    // 清理过期帧数据，保留最近 N 帧
    void Gc(uint32_t current_frame_index, uint32_t keep_frame_count = 10);

private:
    std::unordered_map<uint32_t, std::unordered_map<uint32_t, ClientInput>> frameInputs_; // key=frame_index, value=该帧的所有指令
    std::shared_mutex mutex_; // 保护 frameInputs_ 的读写锁
};