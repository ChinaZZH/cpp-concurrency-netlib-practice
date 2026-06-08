
#include "EventLoopThread.h"
#include "EventLoop.h"
#include <cassert>
#include <iostream>

 EventLoopThread::EventLoopThread(const ThreadInitCallback& cb /*= ThreadInitCallback()*/, const std::string& name /*= std::string()*/)
 :loop_(nullptr)
 ,exiting_(false)
 ,callback_(cb)
 ,name_(name)
 {

 }

 EventLoopThread::~EventLoopThread()
 {
    exiting_ = true;
    if(loop_)
    {
        loop_->Quit();
        if(thread_ && thread_->joinable())
        {
            thread_->join();
        }
    }
 }


// 启动线程后，返回该线程中的EventLoop指针
EventLoop* EventLoopThread::StartLoop()
{
    assert(!thread_);
    thread_ = std::make_unique<std::thread>(&EventLoopThread::ThreadFunc, this);
    {
        std::unique_lock<std::mutex> lock(mutex_);
        cond_.wait(lock, [this]() { return loop_ != nullptr; });
    }

    return loop_;
}
    

 // 线程入口函数
void EventLoopThread::ThreadFunc()
{
    //std::cout << "EventLoopThread::ThreadFunc::111" << std::endl;
    EventLoop event_loop;
    //std::cout << "EventLoopThread::ThreadFunc::222" << std::endl;
    if(callback_)
    {
        callback_(&event_loop);
    }
    //std::cout << "EventLoopThread::ThreadFunc::3333" << std::endl;


    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &(event_loop);
        cond_.notify_one();
    }

    //std::cout << "EventLoopThread::ThreadFunc::4444" << std::endl;
    event_loop.Loop();
    //std::cout << "EventLoopThread::ThreadFunc::5555" << std::endl;
    
    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = nullptr;
    }
} 