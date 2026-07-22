#include "FrameScheduler.h"
#include <chrono>
#include "../../../build/proto_gen/frame_sync.pb.h"
#include "ServerPlayerManager.h"

// 每隔50毫秒打包一次
 void FrameScheduler::OnTick()
 {
    // 1. 推进帧号 + 清理过期缓存
    uint32_t server_frame_index = AddServerFrameIndex();
    buffer_->Gc(server_frame_index);

    // 2. 从缓冲器取当前帧指令
    auto inputs = buffer_->FetchFrame(server_frame_index);

    // 3. 构造帧包（即使空包也要推进帧号）
    uint64_t currentTime = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count());

    FramePackage framePkg;
    framePkg.set_frame_index(server_frame_index);
    framePkg.set_timestamp_ms(currentTime);
    for(auto& input : inputs) 
    {
        auto* newInput = framePkg.add_inputs();
        *newInput = input; // protobuf 支持直接赋值
        //std::cout << "FrameScheduler::OnTick x:=" << newInput->move_x() << " y:=" << newInput->move_y() <<std::endl;
    }
    
    const auto& allPlayer = serverPlayerMgr_->GetAllPlayersState();
    for(const auto& [playerId, serverPlayerState] : allPlayer)
    {
        PlayerState* playerState = framePkg.add_states();
        playerState->set_player_id(playerId);
        playerState->set_x(serverPlayerState.x.Raw());
        playerState->set_y(serverPlayerState.y.Raw());
    }
    

    // 4. 序列化并广播
    std::string serialized_pkg;
    if(framePkg.SerializeToString(&serialized_pkg))
    {
        if(broadcastCallback_)
        {       
            broadcastCallback_(serialized_pkg);
        }       
    }
 }


 uint32_t FrameScheduler::AddServerFrameIndex()
 {
    return current_frame_index_.fetch_add(1, std::memory_order_relaxed);
 }