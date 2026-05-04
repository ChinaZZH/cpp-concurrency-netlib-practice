#include "EventLoop.h"
#include "Channel.h"
#include "Epoll.h"
#include <cassert>
#include <iostream>


EventLoop::EventLoop()
:epoll_(std::make_unique<Epoll>())
, quit_(false)
,threadId_(std::this_thread::get_id())
{
   
}

EventLoop::~EventLoop() = default;

void EventLoop::Loop()
{
    // 5.接受连接并echo
    AssertInLoopThread();
    while(!quit_)
    {
        std::vector<Channel*> activeChannels;
        errno = 0;
        int nfds = epoll_->Wait(activeChannels);
        if(nfds < 0)
        {
            if (errno != EINTR) {
                std::cerr << "epoll_wait error\n";
                break;
            }

            continue;
        }

        for(auto& ptrChannel: activeChannels)
        {
            if(!ptrChannel)
            {
                continue;
            }

            ptrChannel->HandleEvent();
        }
    }
}


void EventLoop::Quit()
{
    quit_ = true;
}

// 添加或更新channel(线程安全)
void EventLoop::UpdateChannel(Channel* channel)
{
    AssertInLoopThread();

    int fd = channel->GetFd();
    auto itr = channels_.find(fd);
    if(itr == channels_.end())
    {
        if(false == epoll_->AddFd(channel))
        {
            std::cerr << "epoll_ctl add fd error listen fd:" << channel->GetFd() << std::endl;
            return ;
        }

        channels_.insert(std::make_pair(channel->GetFd(), channel));

    }else{
        if(false == epoll_->ModifyFd(channel))
        {
            std::cerr << "epoll_ctl modify fd error listen fd:" << channel->GetFd() << std::endl;
            return ;
        }
    }
}


// 移除Channel
void EventLoop::RemoveChannel(Channel* channel)
{
    AssertInLoopThread();
    int fd = channel->GetFd();
    auto itr = channels_.find(fd);
    if(itr != channels_.end())
    {
        epoll_->RemoveFd(channel->GetFd());
        channels_.erase(channel->GetFd());
    }
}


void EventLoop::AssertInLoopThread()
{
    assert(threadId_ == std::this_thread::get_id());
}

bool EventLoop::IsInLoopThread() const
{
    return threadId_ == std::this_thread::get_id();
}
