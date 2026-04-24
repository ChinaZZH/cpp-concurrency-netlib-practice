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
#include "MpmcQueue.h"


class ThreadPool
{
public:
	explicit ThreadPool(int threadCount)
		: m_bStop(false)
		, m_queue(1024)
	{
		m_works.reserve(threadCount);
		for(int i = 0; i < threadCount; ++i)
		{
			m_works.emplace_back([this]() {
				while(true)
				{
					std::function<void()> task;
					{
						if(m_queue.dequeue(task))
						{
							task();
						}
						else if(m_bStop.load(std::memory_order_acquire))
						{
							return;
						}
						else {
							// 队列空，让出CPU
							std::this_thread::yield();
						}
					}
				}
			});
		}
	}


	~ThreadPool()
	{
		m_bStop.store(true, std::memory_order_release);
		for(auto& th : m_works)
		{
			if(th.joinable())
			{
				th.join();
			}
		}
	}

	template<typename F, typename... Args>
	auto AddTask(F&& f, Args&&... args) 
		-> std::future<std::invoke_result_t<F, Args...>>
	{
		// 已经结束不加task
		if(m_bStop.load(std::memory_order_acquire))
		{
			throw std::runtime_error("enqueue on stopped ThreadPool");
		}

		using return_type = std::invoke_result_t<F, Args...>;

		// 将用户函数f和参数args绑定成一个无参的可调用函数
		auto bound_func = std::bind(std::forward<F>(f), std::forward<Args>(args)...);

		// 用绑定后的函数构造一个packaged_task ，他返回return_type
		std::packaged_task<return_type()> task_obj(bound_func);

		auto task = std::make_shared<std::packaged_task<return_type()>>(std::move(task_obj));

		std::future<return_type> res = task->get_future();

		// 循环入队，直到成功
		while(!m_queue.enqueue([task]() { (*task)(); }))
		{
			// 队列满，先让出CPU
			std::this_thread::yield();
		}

		return res;
	}

private:
	std::vector<std::thread> m_works;
	std::atomic<bool> m_bStop = false;

	MPMC_Queue<std::function<void()>> m_queue;
};



int main() {
	ThreadPool pool(4);

	const int N = 10000;
	std::vector<std::future<int>> results;

	auto start = std::chrono::steady_clock::now();
	for(int i = 0; i < N; ++i)
	{
		results.emplace_back(pool.AddTask([i] { return i * i; }));
	}


	long long sum = 0;
	for(auto& fut: results)
	{
		sum += fut.get();
	}

	auto end = std::chrono::steady_clock::now();
	std::cout << "Sum: " << sum << std::endl;
	std::cout << "Time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << " ms" << std::endl;
	
	return 0;
}
