#include "RemotePlayerSmoother.h"
#include <iostream>

 // 1. 推送新状态（由网络层收到服务器广播时调用）
void RemotePlayerSmoother::PushState(const RemoteStateSnapshot& newState)
{
     // 如果新状态无效，直接忽略
    if(false == newState.valid)
    {
        return;
    }


    // 如果当前还没有任何有效状态，直接作为 prev 存储，等待下一个状态
    if(false == prev_.valid && false == next_.valid)
    {
        prev_ = newState;
        prev_.valid = true;
        return;
    }

    // 如果只有一个有效状态（prev_有效，next_无效）
    if(prev_.valid && false == next_.valid)
    {
        // 如果新状态时间戳大于 prev_，则作为 next_ 存储
        if(newState.server_frame > prev_.server_frame)
        {
            next_ = newState;
            next_.valid = true;
             //std::cout << "RemotePlayerSmoother::PushState 11111 x:=" << next_.x.Raw() <<  std::endl;
        }
        else
        {
            // 如果新状态时间戳更早或相同，忽略（可能是重传的旧包）
            return;
        }

        return;
    }

    // 如果双缓冲都有效（prev_ 和 next_ 都存在）
    if(prev_.valid && next_.valid)
    {
         // 如果新状态时间戳大于 next_，则滑动窗口：prev = next, next = new
        if(newState.server_frame > next_.server_frame)
        {
            prev_ = next_;
            next_ = newState;
           // std::cout << "RemotePlayerSmoother::PushState 2222 x:=" << next_.x.Raw() <<  std::endl;
        }
        else if(newState.server_frame > prev_.server_frame){
            // 如果新状态时间戳小于 next_ 但大于 prev_，则替换 next_（更好的一步到位）
            next_ = newState;
           // std::cout << "RemotePlayerSmoother::PushState 333 x:=" << next_.x.Raw() <<  std::endl;
        }
    }
}


// 线性插值辅助函数
Fixed RemotePlayerSmoother::Lerp(Fixed a, Fixed b, float t) const
{
    // 确保 t 在 [0, 1] 范围内
    if(t < 0.0f)
    {
        t = 0.0f;
    }

    if(t > 1.0f)
    {
        t = 1.0f;
    }

    // 线性差值
    Fixed diff = b - a;
    float extraAdd = ((diff.Raw() >= 0.0f)? 0.5f : -0.5f);
    Fixed::RawType result = diff.Raw() * t + extraAdd; 
    return a + Fixed::FromRaw(result);
}


// 2. 获取用于渲染的插值状态（由渲染循环调用）
RemoteStateSnapshot RemotePlayerSmoother::GetRenderState(uint32_t render_frame) const
{
    // 如果双缓冲无效，返回空状态
    if(false == prev_.valid || false == next_.valid)
    {
        RemoteStateSnapshot empty;
        empty.valid = false;
       // std::cout << "RemotePlayerSmoother::GetRenderState 1111" << std::endl;
        return empty;
    }

    // 如果当前时间早于 prev_ 时间，返回 prev_
    if(render_frame < prev_.server_frame)
    {
        RemoteStateSnapshot empty;
        empty.valid = false;
       // std::cout << "RemotePlayerSmoother::GetRenderState 2222" << std::endl;
        return empty;
    }

    // 如果两个状态时间戳相同，直接返回 prev_
    if(prev_.server_frame == next_.server_frame)
    {
        return prev_;
    }

     

    // 如果当前时间晚于 next_ 时间，返回 next_（或继续插值到未来，但会超出范围）
    // 一般做法：如果 now > next，则 t > 1，直接返回 next_。
    if(render_frame > next_.server_frame)
    {
        //std::cout << "RemotePlayerSmoother::GetRenderState 333 x:=" << next_.x.ToFloat() <<  std::endl;
        return next_;
    }

    // 计算插值因子 t = (now - prev_time) / (next_time - prev_time)
    uint32_t total_frame_diff = next_.server_frame - prev_.server_frame;
    uint32_t frame_since_prev = render_frame - prev_.server_frame;
    float fPercent = static_cast<float>(frame_since_prev) / static_cast<float>(total_frame_diff);

    // 执行插值
    RemoteStateSnapshot result;
    result.x = Lerp(prev_.x, next_.x, fPercent);
    result.y = Lerp(prev_.y, next_.y, fPercent);
    result.server_frame = render_frame;
    result.valid = true;

    return result;
}
