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
#include "../Common/TimeWheel.h"


class PartitionedPool
{
public:
	struct PartitionedData
	{
		moodycamel::BlockingConcurrentQueue<std::function<void()>> blockingQueue;
		TimeWheel timer;
	};

	PartitionedPool()
	: m_bStop(false)
	{
    
	}


	~PartitionedPool()
	{
		{
			m_bStop.store(true, std::memory_order_release);
		}

		for(auto& worker : m_works)
		{
			if(worker.joinable())
			{
				worker.join();
			}
		}
	}


    int GetParitionedCount() const { return m_works.size(); }
    


	void Start(int threadNum)
	{
    	m_works.reserve(threadNum);
		m_data.reserve(threadNum);
		for(int i = 0; i < threadNum; ++i)
		{
			std::unique_ptr<PartitionedData> ptrData = std::make_unique<PartitionedData>();
			m_data.push_back(std::move(ptrData));
		}

		for(int i = 0; i < threadNum; ++i)
		{
			int thread_index = i;
			m_works.emplace_back([thread_index, this]() {
            	while(true){
					// 最多500毫秒都RunNextTick
					m_data[thread_index]->timer.RunNexTick();

					std::function<void()> task;
					if(m_data[thread_index]->blockingQueue.wait_dequeue_timed(task, std::chrono::milliseconds(500)))
                    {
                        if(task){
							task();
						}
                    }
					else if(m_bStop.load(std::memory_order_acquire))
					{
						// 结束现成函数
						return;
					}
                }
			});
		}
	}


	
	bool CommitTask(int idx, std::function<void()> funcCallBack) 
    {
        // 已经结束不加task
		if(m_bStop)
		{
			throw std::runtime_error("enqueue on stopped ThreadPool");
		}

        if (idx < 0 || static_cast<size_t>(idx) >= m_data.size()) {
             throw std::runtime_error("enqueue on idx out of range");
        }


		m_data[idx]->blockingQueue.enqueue(funcCallBack);
		return true;
    }

	void CancelTimer(int idx, uint64_t cancelTimerId) 
	{
		 // 已经结束不加task
		if(m_bStop)
		{
			throw std::runtime_error("enqueue on stopped ThreadPool");
		}

        if (idx < 0 || static_cast<size_t>(idx) >= m_data.size()) {
             throw std::runtime_error("enqueue on idx out of range");
        }


		m_data[idx]->timer.CancelTimer(cancelTimerId);
	}

	uint64_t DelayRunOnce(int idx, int delaySeconds, std::function<void()> funcCallBack) 
	{
		return DelayRunLoopCount(idx, delaySeconds, funcCallBack, 1);
	}


	uint64_t DelayRunEvery(int idx, int delaySeconds, std::function<void()> funcCallBack) 
	{
		return DelayRunLoopCount(idx, delaySeconds, funcCallBack, 0);
	}

	uint64_t DelayRunLoopCount(int idx, int delaySeconds, std::function<void()> funcCallBack, int delayCount) 
	{
		 // 已经结束不加task
		if(m_bStop)
		{
			throw std::runtime_error("enqueue on stopped ThreadPool");
		}

        if (idx < 0 || static_cast<size_t>(idx) >= m_data.size()) {
             throw std::runtime_error("enqueue on idx out of range");
        }


		return m_data[idx]->timer.AddNewTimer(delaySeconds, delayCount, funcCallBack);
	}


private:
	std::vector<std::thread> m_works;
	std::vector<std::unique_ptr<PartitionedData>> m_data;
    std::atomic<bool> m_bStop = false;
};


