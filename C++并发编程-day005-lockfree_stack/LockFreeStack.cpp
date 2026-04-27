
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

template<typename T>
class LockFreeStack
{
private:
	struct Node
	{
		T data;
		Node* next;
		
		Node(const T& val) :data(val), next(nullptr) {} 
	};

	std::atomic<Node*> head = nullptr;

public:
	LockFreeStack() = default;
	
	~LockFreeStack()
	{

	}

	void Push(const T& val)
	{
		// 写法1
		{
			Node* new_node = new Node(val);
			new_node->next = head.load(std::memory_order_relaxed);

			while (!head.compare_exchange_weak(new_node->next,
				new_node,
				std::memory_order_release,
				std::memory_order_acquire)) { } // loop
		}
		
		// 写法2
		// 同时上面的写法可以同时这么写
		/*
		{
			Node* new_node = new Node(val);
			Node* head_node = head.load(std::memory_order_relaxed);
			do
			{
				new_node->next = head_node;
			} while (!head.compare_exchange_weak(head_node,
				new_node,
				std::memory_order_release,
				std::memory_order_acquire)); // loop
		}
		*/
	}

	bool Pop(T& val)
	{
		Node* head_node = head.load(std::memory_order_acquire);
		while(head_node && !head.compare_exchange_weak(head_node,
			head_node->next, std::memory_order_acq_rel, std::memory_order_acquire))  { } // loop
		
		if(!head_node)
		{
			return false;
		}

		val = head_node->data;
		delete head_node;
		return true;
	}

	bool empty()
	{
		Node* head_node = head.load(std::memory_order_acquire);
		return (nullptr == head_node);
	}
};


// 测试代码
int main()
{
	LockFreeStack<int> stack;
	const int N = 10;
	const int PRODUCERS = 4;
	const int CONSUMERS = 4;
	
	std::atomic<int> sum = 0;
	std::atomic<int> produced_count = 0;
	std::atomic<int> consumed_count = 0;

	auto func_producer = [&](int start) {
		for(int i = 0; i < N; ++i)
		{
			stack.Push(start+i);
			++produced_count;
		}
	};

	auto func_consumer = [&]() {
		int value = 0;
		while(consumed_count < PRODUCERS*N)
		{
			if(stack.Pop(value))
			{
				sum += value;
				++consumed_count;
			}
		}
	};


	std::vector<std::thread> producers_thread_list;
	for(int i  = 0; i < PRODUCERS; ++i)
	{
		producers_thread_list.emplace_back(func_producer, i*N);
	}

	std::vector<std::thread> consumers_thread_list;
	for(int i = 0; i < CONSUMERS; ++i)
	{
		consumers_thread_list.emplace_back(func_consumer);
	}

	for (auto& pro_thread : producers_thread_list) pro_thread.join();
	for (auto& consume_thread : consumers_thread_list) consume_thread.join();

	std::cout << "Produced: " << produced_count << std::endl;
	std::cout << "Consumed: " << consumed_count << std::endl;
	std::cout << "Sum: " << sum << std::endl;
	return 0;

}