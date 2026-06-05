#include "EventLoopThreadPool.h"
#include "EventLoop.h"
#include "EventLoopThread.h"
#include <cassert>
#include <iostream>

EventLoopThreadPool::EventLoopThreadPool(EventLoop* baseLoop, const std::string& nameArg /*=""*/)
:baseLoop_(baseLoop)
,name_(nameArg)
,started_(false)
,numThreads_(0)
,next_(0)
{
    
}


EventLoopThreadPool::~EventLoopThreadPool()
{
    // 不主动调用baseLoop->Quit 不主动销毁 baseLoop_，它由外部管理
}

void EventLoopThreadPool::Start(TcpServer* tcpServer, int idleSecTimeOut, const ThreadInitallback& cb /*= ThreadInitallback()*/)
{
    assert(!started_); 
    baseLoop_->AssertInLoopThread("EventLoopThreadPool::Start");
    
    started_ = true;
    for(int i = 0; i < numThreads_; ++i)
    {
        char buf[32] = {0};
        snprintf(buf, sizeof(buf), "%s%d", name_.c_str(), i+1);

        //std::cout << "EventLoopThreadPool::Start 1111" << std::endl;
        auto event_loop_thread = std::make_unique<EventLoopThread>(cb, buf);
        EventLoop* loop = event_loop_thread->StartLoop();
        //std::cout << "EventLoopThreadPool::Start 2222" << std::endl;
        if(loop)
        {
            threads_.emplace_back(std::move(event_loop_thread));
            
            loop->InitServer(tcpServer);
            if(idleSecTimeOut > 0)
            {
                loop->StartIdleConnectionSecsTimeOut(idleSecTimeOut);
            }
            
            loops_.emplace_back(loop);
        }
    }

    // 如果没有创建子线程但是提供了回调，则再baseLoop上执行
    if(0 == numThreads_)
    {
        baseLoop_->StartIdleConnectionSecsTimeOut(idleSecTimeOut);
        if(cb)
        {
            cb(baseLoop_);
        }
    }
}


EventLoop* EventLoopThreadPool::getNextLoop()
{
    if(!this->IsStarted())
    {
        return baseLoop_;
    }

    if(loops_.size() <= 0)
    {
        return baseLoop_;
    }


    int index = next_ % loops_.size();
    if(0 == index)
    {
        next_ = 0;
    }

    next_ += 1;
    return loops_[index];
}


std::vector<EventLoop*> EventLoopThreadPool::GetAllLoops() const
{
    return loops_;
}