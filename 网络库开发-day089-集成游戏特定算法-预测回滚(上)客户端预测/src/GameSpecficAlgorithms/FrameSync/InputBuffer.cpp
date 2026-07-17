#include "InputBuffer.h"
#include <shared_mutex>

 // 由 TcpServer OnMessage 调用（多线程安全）
void InputBuffer::PushInput(uint32_t playerId, uint32_t server_frame, const ClientInput& inputData)
{
    //std::cout << "InputBuffer::PushInput playerID:=" << playerId << std::endl; 
    std::unique_lock<std::shared_mutex> lock(mutex_);
    // 【关键修正】使用服务端传入的 server_frame 作为外层 Key，完全忽略 input.frame_index()
    auto& player_map = frameInputs_[server_frame];
    player_map[playerId] = inputData;  // 同帧同玩家覆盖
}

// 由帧调度器调用（单线程），取出指定帧的所有指令
std::vector<ClientInput> InputBuffer::FetchFrame(uint32_t frame_index)
{
    std::vector<ClientInput> result;
    std::unique_lock<std::shared_mutex> lock(mutex_);
    auto it = frameInputs_.find(frame_index);
    if (it == frameInputs_.end())
    {
        return result;
    }
    
    result.reserve(it->second.size());
    for(auto& kv : it->second) {
        // 修正 frame_index 为服务端帧号（以防客户端乱填）
        ClientInput copy = kv.second;
        copy.set_frame_index(frame_index);
        result.push_back(std::move(copy));
    }

    return result;
}

// 清理过期帧数据，保留最近 N 帧
void InputBuffer::Gc(uint32_t current_frame_index, uint32_t keep_frame_count /*= 10*/)
{
    uint32_t min_frame_index_to_keep = (current_frame_index > keep_frame_count) ? (current_frame_index - keep_frame_count) : 0;
    std::unique_lock<std::shared_mutex> lock(mutex_);
    for (auto it = frameInputs_.begin(); it != frameInputs_.end(); )
    {
        if((it->first) >= min_frame_index_to_keep)
        {
            ++it;
        }
        else{
            it = frameInputs_.erase(it);
        }
    }
}