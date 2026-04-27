
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
	Node* next;

	Node(int val) :data(val), next(nullptr) {}
};


// 指针 + 版本号
struct TaggedPtr
{
	Node* node_ptr;
	uint64_t tag;

	TaggedPtr(Node* ptr = nullptr, uint64_t t = 0) :node_ptr(ptr), tag(t) {}
};

static_assert(std::is_trivially_copyable<TaggedPtr>::value, "Must be trivially copable");

// 固定大小的对象池，保证地址可以重用(内存池)
class TaggedStack
{
public:
	TaggedStack() = default;
	
	~TaggedStack()
	{
		int tmp = 0;
		while(pop(tmp)) { }
	}

	void push(int val)
	{
		Node* new_node = new Node(val);

		TaggedPtr old_head = head_.load(std::memory_order_relaxed);
		TaggedPtr new_head;
		do
		{
			new_node->next = old_head.node_ptr;
			// 将old_head.tag + 1修改为old_head.tag的时候程序会产生ABA情况并且程序崩溃
			new_head = TaggedPtr(new_node, old_head.tag + 1); 
		} while (!head_.compare_exchange_weak(
			old_head, 
			new_head,
			std::memory_order_release, 
			std::memory_order_relaxed));
	}


	bool pop(int& val)
	{
		TaggedPtr old_head = head_.load(std::memory_order_acquire);
		while(old_head.node_ptr)
		{
			// 将old_head.tag + 1修改为old_head.tag的时候程序会产生ABA情况并且程序崩溃
			TaggedPtr new_head(old_head.node_ptr->next, old_head.tag + 1);
			if(head_.compare_exchange_weak(
				old_head,
				new_head,
				std::memory_order_acq_rel,
				std::memory_order_relaxed))
			{
				val = old_head.node_ptr->data;
				delete old_head.node_ptr;
				return true;
			}
		} 
		
		return false;
	}

private:
	std::atomic<TaggedPtr> head_; // 只有头结点有版本号，其他结点没有版本号
};


// 多线程压测
int main()
{
	const int N = 10000;		// 每个线程的操作次数
	const int PRODUCERS = 4;	// 生产者线程数
	const int CONSUMES	= 4;	// 消费者线程数

	TaggedStack stack;
	std::atomic<long long> total_pushed = 0;
	std::atomic<long long> total_poped = 0;

	/*
	多线程压测：
	多个生产者、多个消费者同时操作栈，最终比较所有弹出值的总和与理论总和的差。
	若相等，说明无数据丢失 / 重复，且无 ABA。
	同时可用 - fsanitize = thread 编译检测数据竞争。
	*/

	auto producer = [&] (int start){
		for(int i = 0; i < N; ++i)
		{
			stack.push(start + i);
			++total_pushed;
		}
	};

	auto consumer = [&] {
		int val = 0;
		for(int i = 0; i < N; ++i)
		{
			// 判断是否有数据，有数据才能pop成功
			while (!stack.pop(val)) { std::this_thread::yield(); }
			
			total_poped += val;
		}
	};

	std::vector<std::thread> vecThreadList;
	for (int i = 0; i < PRODUCERS; ++i)
	{
		vecThreadList.emplace_back(producer, i*N);
	}

	for (int i = 0; i < CONSUMES; ++i)
	{
		vecThreadList.emplace_back(consumer);
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

