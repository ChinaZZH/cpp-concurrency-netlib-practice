#pragma once

#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <future>
#include <functional>
#include <memory>
#include <type_traits>
#include "../Common/BlockingConcurrentQueue.h"


class PartitionedPool
{
public:
	PartitionedPool()
	: m_bStop(false)
	{
    
	}


	~PartitionedPool()
	{
		{
			m_bStop.store(true, std::memory_order_release);
		}

		for(auto& th : m_works)
		{
			if(th.joinable())
			{
				th.join();
			}
		}
	}


    int GetParitionedCount() const { return m_blockingConcurrentQueue.size(); }
    
	void Start(int threadNum)
	{
    	m_works.reserve(threadNum);
        m_blockingConcurrentQueue.reserve(threadNum);
        for(int i = 0; i < threadNum; ++i)
        {
            m_blockingConcurrentQueue.emplace_back(moodycamel::BlockingConcurrentQueue<std::function<void()>>());
        }
        

		for(int i = 0; i < threadNum; ++i)
		{
            int queueIndex = i;  // 显式保存
			m_works.emplace_back([this, queueIndex]() {
            	while(!m_bStop.load(std::memory_order_acquire)){
						std::function<void()> task;
						if(m_blockingConcurrentQueue[queueIndex].wait_dequeue_timed(task, std::chrono::milliseconds(500)))
                        {
                            if(task){
							    task();
						    }
                        }
                    }
			});

		}
	}


	template<typename F, typename... Args>
	auto CommitTask(int idx, F&& f, Args&&... args) 
		-> std::future<std::invoke_result_t<F, Args...>>
    {
        // 已经结束不加task
		if(m_bStop)
		{
			throw std::runtime_error("enqueue on stopped ThreadPool");
		}

        if (idx < 0 || static_cast<size_t>(idx) >= m_blockingConcurrentQueue.size()) {
             throw std::runtime_error("enqueue on idx out of range");
        }

		using return_type = std::invoke_result_t<F, Args...>;
		/*
		auto task = std::make_shared<std::packaged_task<return_type()>>(
			std::bind(std::forward<F>(f), std::forward<Args>(args)...)
		);
		*/

		// 将用户函数f和参数args绑定成一个无参的可调用函数
		auto bound_func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);

		// 用绑定后的函数构造一个packaged_task ，他返回return_type
		std::packaged_task<return_type()> task_obj(bound_func);

		auto task = std::make_shared<std::packaged_task<return_type()>>(std::move(task_obj));

		std::future<return_type> res = task->get_future();

		m_blockingConcurrentQueue[idx].enqueue([task]() { (*task)(); });
		return res;
    }

private:
	std::vector<std::thread> m_works;
    std::vector<moodycamel::BlockingConcurrentQueue<std::function<void()>>> m_blockingConcurrentQueue;
    std::atomic<bool> m_bStop = false;
};


