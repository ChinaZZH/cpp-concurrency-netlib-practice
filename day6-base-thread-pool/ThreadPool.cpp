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


class ThreadPool
{
public:
	ThreadPool(int threadCount) : m_bStop(false)
	{
		m_works.reserve(threadCount);
		for(int i = 0; i < threadCount; ++i)
		{
			m_works.emplace_back([this]() {
				while(true)
				{
					std::function<int()> task;
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
						int val = task();
						std::cout << val << std::endl;
					}
				}
			});
		}
	}


	~ThreadPool()
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

	void AddTask(const std::function<int()>& task)
	{
		// 已经结束不加task
		if(m_bStop)
		{
			return;
		}

		{
			std::unique_lock<std::mutex> lk(m_mtx);
			m_tasks.emplace(task);
		}

		m_cv.notify_one();
	}

private:
	std::vector<std::thread> m_works;
	std::queue<std::function<int()>> m_tasks;
	bool m_bStop = false;

	std::mutex m_mtx;
	std::condition_variable m_cv;
};


int add(int a, int b)
{
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	return a + b;
}


int main()
{
	ThreadPool pool(4);

	// 提交多个任务，获取future
	pool.AddTask([]() { return 11; });
	pool.AddTask([]() { return 22; });
	pool.AddTask([]() { return 33; });

	
	pool.AddTask([]() { return 44; });
	pool.AddTask([]() { return 55; });
	pool.AddTask([]() { return 66; });

	pool.AddTask([]() { return 77; });
	pool.AddTask([]() { return 88; });
	pool.AddTask([]() { return 99; });

	return 0;
}