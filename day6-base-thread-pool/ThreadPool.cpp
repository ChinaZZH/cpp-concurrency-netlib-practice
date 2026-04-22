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

	template<typename F, typename... Args>
	auto AddTask(F&& f, Args&&... args) 
		-> std::future<std::invoke_result_t<F, Args...>>
	{
		// 已经结束不加task
		if(m_bStop)
		{
			throw std::runtime_error("enqueue on stopped ThreadPool");
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

		{
			std::unique_lock<std::mutex> lk(m_mtx);
			m_tasks.emplace([task]() { (*task)(); });
		}

		m_cv.notify_one();
		return res;
	}

private:
	std::vector<std::thread> m_works;
	std::queue<std::function<void()>> m_tasks;
	bool m_bStop = false;

	std::mutex m_mtx;
	std::condition_variable m_cv;
};


int add(int a, int b) {
	std::this_thread::sleep_for(std::chrono::milliseconds(100));
	return a + b;
}

int main() {
	ThreadPool pool(4);

	// 提交多个任务，获取 future
	auto f1 = pool.AddTask(add, 10, 20);
	auto f2 = pool.AddTask([](int x, int y) { return x * y; }, 5, 6);
	auto f3 = pool.AddTask([]() { return 42; });

	// 获取结果
	std::cout << f1.get() << std::endl;
	std::cout << f2.get() << std::endl;
	std::cout << f3.get() << std::endl;

	return 0;
}