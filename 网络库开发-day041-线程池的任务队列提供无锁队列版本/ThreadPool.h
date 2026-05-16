#pragma once

#include "LibThreadPool/ThreadPool_Mutex.h"
#include "LibThreadPool/ThreadPool_ConcurrentQueue.h"
#include "LibThreadPool/ThreadPool_BlockingConcurrentQueue.h"
#include "LibThreadPool/ThreadPool_HazardMpmcQueue.h"

// 支持多种线程同步方式
class ThreadPool
{
public:
    ThreadPool()
    { }

    void Start(int option, int threadCount)
    {
        option_ = option;
        if(0 == option_)
        {
            mutex_.Start(threadCount);
        }
        else if(1 == option_)
        {
            concurrentQueue_.Start(threadCount);
        }
        else if(2 == option_)
        {
            blockingConcurrentQueue_.Start(threadCount);
        }
        else
        {
            hazardMpmcQueue_.Start(threadCount);
        }
    }


    template<typename F, typename... Args>
	auto AddTask(F&& f, Args&&... args) 
		-> std::future<std::invoke_result_t<F, Args...>>
    {
        if(0 == option_)
        {
            return mutex_.AddTask(std::forward<F>(f), std::forward<Args>(args)...);
        }
        else if(1 == option_)
        {
            return concurrentQueue_.AddTask(std::forward<F>(f), std::forward<Args>(args)...);
        }
        else if(2 == option_){
           return blockingConcurrentQueue_.AddTask(std::forward<F>(f), std::forward<Args>(args)...);
        }
        else{
            return hazardMpmcQueue_.AddTask(std::forward<F>(f), std::forward<Args>(args)...);
        }
       
    }

private:
    int option_;

    ThreadPool_Mutex mutex_;            // 互斥锁
    ThreadPool_ConcurrentQueue              concurrentQueue_;               // 工业级无锁队列
    ThreadPool_BlockingConcurrentQueue      blockingConcurrentQueue_;       // 工业级无锁队列
    ThreadPool_HazardMpmcQueue              hazardMpmcQueue_;               // 自己实现教学使用的无锁队列
};