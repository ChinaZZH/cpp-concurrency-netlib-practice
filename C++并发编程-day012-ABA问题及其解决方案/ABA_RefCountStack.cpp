
#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>
#include <vector>
#include <stack>
#include <cassert>


struct Node
{
	int data;
	std::shared_ptr<Node> next;  // 引用计数 shared_ptr 相当于一个隐形的版本号的作用
	Node(int val) :data(val), next(nullptr) {}
};



class RefCountStack
{
public:
	void push(int val)
	{
		auto new_head = std::make_shared<Node>(val);
		auto old_head = head_.load(std::memory_order_relaxed);
		do
		{
			new_head->next = old_head;
		} while(head_.compare_exchange_weak(old_head, new_head, std::memory_order_release, std::memory_order_relaxed));
	}


	bool pop(int& val)
	{
		auto old_head = head_.load(std::memory_order_acquire);
		while(old_head)
		{
			auto new_head = old_head->next;
			if(head_.compare_exchange_weak(old_head, new_head, std::memory_order_acq_rel, std::memory_order_relaxed))
			{
				val = old_head->data;
				// 引用为0的时候会自动删除
				return true;
			}
		}

		std::this_thread::yield();   // 让出 CPU，避免空转
		return false;
	}

private:
	std::atomic<std::shared_ptr<Node>> head_; // 只有头结点有版本号，其他结点没有版本号
};





int main()
{
	//std::cout << "Program started" << std::endl;   // 立即输出

	const int N = 100;		// 每个线程的操作次数
	const int PRODUCERS = 2;	// 生产者线程数
	const int CONSUMES	= 2;	// 消费者线程数
	// 
	// 多线程压测

	/*
	多线程压测：
	多个生产者、多个消费者同时操作栈，最终比较所有弹出值的总和与理论总和的差。
	若相等，说明无数据丢失 / 重复，且无 ABA。
	同时可用 - fsanitize = thread 编译检测数据竞争。
	*/

	RefCountStack stack;
	std::atomic<long long> total_pushed = 0;
	std::atomic<long long> total_poped = 0;

	

	auto producer = [&] (int start){
		//std::cout << "Producer started with start=" << start << std::endl;
		for(int i = 0; i < N; ++i)
		{
			stack.push(start + i);
			++total_pushed;
		}
		//std::cout << "Producer finished" << std::endl;
	};

	auto consumer = [&] {
		//std::cout << "Consumer started" << std::endl;
		int val = 0;
		for(int i = 0; i < N; ++i)
		{
			// 判断是否有数据，有数据才能pop成功
			while (!stack.pop(val)) { std::this_thread::yield(); }
			
			total_poped += val;
		}
		//std::cout << "Consumer finished" << std::endl;
	};

	//std::cout << "Creating threads..." << std::endl;
	std::vector<std::thread> vecThreadList;
	for (int i = 0; i < PRODUCERS; ++i)
	{
		vecThreadList.emplace_back(producer, i*N);
		//std::cout << "Producer thread " << i << " created" << std::endl;
	}

	for (int i = 0; i < CONSUMES; ++i)
	{
		vecThreadList.emplace_back(consumer);
		//std::cout << "Consumer thread " << i << " created" << std::endl;
	}

	for(auto& th : vecThreadList)
	{
		th.join();
	}

	std::cout << "Total pushed counts: " << total_pushed << std::endl;
	std::cout << "Total popped sum   : " << total_poped << std::endl;

	// 期望总和 = 0 + 1 + ... + (N*PRODUCERS-1) 
	long long expected = (long long)N * PRODUCERS * (N * PRODUCERS - 1) / 2;
	std::cout << "Expected sum        : " << expected << std::endl;

	if (total_poped == expected) {
		std::cout << "✅ Test passed: no data race or ABA detected." << std::endl;
	}
	else {
		std::cout << "❌ Test failed: sum mismatch, possible ABA or data race." << std::endl;
	}

	return 0;
}


