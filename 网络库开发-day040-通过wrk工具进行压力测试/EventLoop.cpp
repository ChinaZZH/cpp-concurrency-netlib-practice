#include "EventLoop.h"
#include "Channel.h"
#include "Epoll.h"
#include "TcpServer.h"
#include <cassert>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include <sys/eventfd.h>


EventLoop::EventLoop()
:epoll_(std::make_unique<Epoll>())
, quit_(false)
,threadId_(std::this_thread::get_id())
, wakeUpFd_(::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC))
, tcpServer_(nullptr)
{

    {
        if(wakeUpFd_ < 0)
        {
            perror("eventfd");
            exit(1);
        }

        auto wakeUpChannel = std::make_unique<Channel>(wakeUpFd_);
        wakeUpChannel->SetReadCallBack([this](){
            uint64_t one = 0;
            ssize_t ret = ::read(wakeUpFd_, &one, sizeof(one));
            (void)ret;
        });

        wakeUpChannel->EnableReading();
        this->AddChannel(std::move(wakeUpChannel));
    }
}

EventLoop::~EventLoop() 
{
    if(wakeUpFd_ >= 0)
    {
        ::close(wakeUpFd_);
    }
}

void EventLoop::Loop()
{
    // 5.接受连接并echo
    AssertInLoopThread("EventLoop::Loop");
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

        DoPendingFunctors();
    }
}


void EventLoop::Quit()
{
    quit_ = true;
    WakeUp();
}

// 添加或更新channel(线程安全)
void EventLoop::AddChannel(std::unique_ptr<Channel> channel, std::string strInfo /*= "OtherError"*/)
{
    AssertInLoopThread("EventLoop::AddChannel");
    int fd = channel->GetFd();
    auto itr = channels_.find(fd);
    if(itr == channels_.end())
    {
        if(false == epoll_->AddFd(channel.get()))
        {
            std::cerr << "epoll_ctl add fd error fd:" << channel->GetFd() << std::endl;
            return ;
        }

        channels_.insert(std::make_pair(channel->GetFd(), std::move(channel)));

    }else{
        std::cerr << "epoll_ctl modify fd error fd:" << channel->GetFd() << "Info" << strInfo << std::endl;
    }
}


// 移除Channel
void EventLoop::RemoveChannel(int fd)
{
    AssertInLoopThread("EventLoop::RemoveChannel");
    auto itr = channels_.find(fd);
    if(itr != channels_.end())
    {
        epoll_->RemoveFd(fd); 
        channels_.erase(fd);
        tcpServer_->RemoveConnectionByFd(fd);
    }
}


void EventLoop::AssertInLoopThread(std::string strInfo)
{
    if(threadId_ != std::this_thread::get_id())
    {
        std::cout << "EventLoop::AssertInLoopThread other thread_id:" << std::this_thread::get_id() << "In Info:=" << strInfo.c_str() << std::endl;
    }

    assert(threadId_ == std::this_thread::get_id());
}

bool EventLoop::IsInLoopThread() const
{
    return threadId_ == std::this_thread::get_id();
}


void EventLoop::DelayRemoveQueue(int fd)
{
    AssertInLoopThread("EventLoop::DelayRemoveQueue");
    // 唤醒fd不通过外部移除
    if(fd == wakeUpFd_)
    {
        return;
    }

    auto itr = channels_.find(fd);
    if(itr != channels_.end())
    {
        auto itrDelay = std::find(delayChannelsToRemove_.begin(), delayChannelsToRemove_.end(), fd);
        if(itrDelay == delayChannelsToRemove_.end())
        {
            delayChannelsToRemove_.push_back(fd);
        }
    }
}


void EventLoop::AddEventToUpdateChannel(int fd, int event)
{
    AssertInLoopThread("EventLoop::AddEventToUpdateChannel");
    auto itr = channels_.find(fd);
    if(itr == channels_.end())
    {
        return ;
    }

    auto& channel = (itr->second);
    channel->EnableEvent(event);
    epoll_->ModifyFd(channel.get());
}


void EventLoop::DelEventToUpdateChannel(int fd, int event)
{
    AssertInLoopThread("EventLoop::DelEventToUpdateChannel");
    auto itr = channels_.find(fd);
    if(itr == channels_.end())
    {
        return ;
    }

    auto& channel = (itr->second);
    channel->DisableEvent(event);
    epoll_->ModifyFd(channel.get());
}


 // 跨线程调度: 如果当前是IO线程则直接执行，否则放入队列
void EventLoop::RunInLoop(std::function<void()> cb)
{
    if(this->IsInLoopThread())
    {
        cb();
    }else{
        QueueInLoop(std::move(cb));
    }
}

void EventLoop::QueueInLoop(std::function<void()> cb)
{
    {
        std::lock_guard<std::mutex> lk(mutex_);
        pendingFunctors_.emplace_back(std::move(cb));
    }
   
    // 可选：唤醒 epoll_wait（简化实现，暂不唤醒，下次事件会处理）
    // 若需要立即唤醒，可写 eventfd
   WakeUp();
}


void EventLoop::DoPendingFunctors()
{
    std::vector<std::function<void()>> tmpFunctorsList;
    {
        std::lock_guard<std::mutex> lk(mutex_);
        if(pendingFunctors_.empty())
        {
            return;
        }
        
        pendingFunctors_.swap(tmpFunctorsList);
    }

    for(auto& func : tmpFunctorsList)
    {
        func();
    }
}

     
void EventLoop::WakeUp()
{
    if(wakeUpFd_ >= 0)
    {
        uint64_t one = 1;
        ssize_t ret = ::write(wakeUpFd_, &one, sizeof(one));
        (void)ret;
    }
    
}