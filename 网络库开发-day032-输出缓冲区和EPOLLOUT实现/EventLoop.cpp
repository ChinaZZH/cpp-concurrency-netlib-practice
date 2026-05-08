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

        if(!delayChannelsToRemove_.empty())
        {
            for(auto& fd : delayChannelsToRemove_)
            {
                this->RemoveChannel(fd);
            }
            
            delayChannelsToRemove_.clear();
        }
    }
}


void EventLoop::Quit()
{
    quit_ = true;
}

// 添加或更新channel(线程安全)
void EventLoop::UpdateChannel(std::unique_ptr<Channel> channel)
{
    AssertInLoopThread();

    int fd = channel->GetFd();
    auto itr = channels_.find(fd);
    if(itr == channels_.end())
    {
        if(false == epoll_->AddFd(channel.get()))
        {
            std::cerr << "epoll_ctl add fd error listen fd:" << channel->GetFd() << std::endl;
            return ;
        }

        channels_.insert(std::make_pair(channel->GetFd(), std::move(channel)));

    }else{
        if(false == epoll_->ModifyFd(channel.get()))
        {
            std::cerr << "epoll_ctl modify fd error listen fd:" << channel->GetFd() << std::endl;
            return ;
        }

        std::cout << "epoll_ctl modify success fd :" << channel->GetFd() << std::endl;
        channels_[channel->GetFd()] = std::move(channel);
    }
}


// 移除Channel
void EventLoop::RemoveChannel(int fd)
{
    AssertInLoopThread();
    auto itr = channels_.find(fd);
    if(itr != channels_.end())
    {
        //epoll_->RemoveFd(fd); 移到DelayRemoveQueue 去处理，以保证顺序 epoll->RemoveFd, ClientSocket析构， Channel析构
        channels_.erase(fd);
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


void EventLoop::DelayRemoveQueue(int fd)
{
    AssertInLoopThread();
    auto itr = channels_.find(fd);
    if(itr != channels_.end())
    {
        auto itrDelay = std::find(delayChannelsToRemove_.begin(), delayChannelsToRemove_.end(), fd);
        if(itrDelay == delayChannelsToRemove_.end())
        {
            delayChannelsToRemove_.push_back(fd);
            epoll_->RemoveFd(fd);
        }
    }
}
