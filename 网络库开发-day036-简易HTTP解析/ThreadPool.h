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


class ThreadPool
{
public:
	ThreadPool(int threadCount);


	~ThreadPool();

    void Start();

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
    int threadNum_;
};


