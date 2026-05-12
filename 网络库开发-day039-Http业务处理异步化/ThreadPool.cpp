#include "ThreadPool.h"





ThreadPool::ThreadPool(int threadCount)
: m_bStop(false)
,threadNum_(threadCount)
{
    
}


ThreadPool::~ThreadPool()
{
	{
		std::unique_lock<std::mutex> lk(m_mtx);
		m_bStop = true;
	}

	m_cv.notify_all();
	for(auto& th : m_works)
	{
		if(th.joinable())
		{
			th.join();
		}
	}
}


void ThreadPool::Start()
{
    m_works.reserve(threadNum_);
	for(int i = 0; i < threadNum_; ++i)
	{
		m_works.emplace_back([this]() {
            while(true)
			{
					std::function<void()> task;
					{
						std::unique_lock<std::mutex> lk(m_mtx);
						m_cv.wait(lk, [&]() { return m_bStop || !m_tasks.empty(); });
						if(m_bStop && m_tasks.empty())
						{
							return;
						}

						task = std::move(m_tasks.front());
						m_tasks.pop();
					}

					if(task)
					{
						task();
					}
			}
		});

	}
}




