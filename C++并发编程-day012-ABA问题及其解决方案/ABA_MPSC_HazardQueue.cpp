
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>
#include <memory>
#include <cstddef>
#include <unordered_map>
#include <unordered_set>

// 节点结构
struct Node
{
	int data;
	std::atomic<Node*>  next;

	Node(int id):data(id),next(nullptr) {}
};


// 全局风险指针(每个消费者一个)
constexpr int MAX_THREADS = 128;
std::atomic<Node*> hazard_ptrs[MAX_THREADS];

// 待回收链表
std::atomic<Node*>  retired_list = nullptr;

// 将节点加入待回收链表
void retire(Node* node)
{
	if(!node)
	{
		return;
	}

	Node* old_head = retired_list.load(std::memory_order_relaxed);
	do {
		node->next = old_head;
	} while(retired_list.compare_exchange_weak(old_head, 
		node, 
		std::memory_order_release, 
		std::memory_order_relaxed)
	);
}


// 回收安全节点（扫描所有的风险指针）
void scan_and_reclaim()
{
	Node* list = retired_list.exchange(nullptr, std::memory_order_acquire);
	if(!list)
	{
		// 没有待回收节点
		return;
	}

	std::vector<Node*> vecHazards;
	for (int i = 0; i < MAX_THREADS; ++i)
	{
		Node* ptr = hazard_ptrs[i].load(std::memory_order_acquire);
		if(ptr)
		{
			vecHazards.emplace_back(ptr);
		}
	}

	Node* it = list;
	while(it)
	{
		Node* next_node = (it->next).load(std::memory_order_acquire);
		bool bSafe = true;
		for(auto& ptr : vecHazards)
		{
			if (ptr == it)
			{
				bSafe = false;
				break;
			}
		}

		if (bSafe)
		{
			delete it;
		}
		else {
			retire(it);
		}


		it = next_node;
	}

}


// Mpsc 队列
class HazardQueue
{
private:
	std::atomic<Node*>  head_;
	std::atomic<Node*>  tail_;
	int thread_id_;				// 当前消费者线程ID (简化，假设只有一个线程)

public:
	// 创建虚拟节点
	HazardQueue():head_(nullptr), tail_(nullptr), thread_id_(0)
	{ 
		Node* dummy = new Node(thread_id_);
		head_.store(dummy, std::memory_order_relaxed);
		tail_.store(dummy, std::memory_order_relaxed);
	}

	~HazardQueue()
	{
		Node* node = head_.load();
		while (node)
		{
			Node* next_node = node->next.load();
			delete node;
			node = next_node;
		}
	}

	// 生产者入队(多线程安全)
	void enqueue(int val)
	{
		Node* new_tail = new Node(val);
		Node* old_tail = tail_.exchange(new_tail, std::memory_order_acq_rel);
		old_tail->next.store(new_tail, std::memory_order_release);
	}

	// 消费者出队列(单线程)
	bool dequeue(int& val, int tid)
	{
		Node* old_head = head_.load(std::memory_order_acquire);
		while(old_head)
		{
			// 设置风险指针为操作节点
			hazard_ptrs[tid].store(old_head, std::memory_order_release);

			// 二次验证
			if(head_.load(std::memory_order_acquire) != old_head)
			{
				hazard_ptrs[tid].store(nullptr, std::memory_order_release);
				old_head = head_.load(std::memory_order_acquire);
				continue;
			}

			Node* node = old_head->next.load(std::memory_order_acquire);
			if (!node)
			{
				// 空队列
				hazard_ptrs[tid].store(nullptr, std::memory_order_release);
				return false;
			}

			if(head_.compare_exchange_weak(old_head, node, std::memory_order_release, std::memory_order_relaxed))
			{
				val = node->data;
				hazard_ptrs[tid].store(nullptr, std::memory_order_release);

				retire(old_head);
				scan_and_reclaim();
				return true;
			}

			// cas失败
			hazard_ptrs[tid].store(nullptr, std::memory_order_release);
		}

		hazard_ptrs[tid].store(nullptr, std::memory_order_release);
		return false;
	}
};



int main()
{
	//std::cout << "Program started" << std::endl;   // 立即输出

	const int N = 100;		// 每个线程的操作次数
	const int PRODUCERS = 4;	// 生产者线程数
	
	// 
	// 多线程压测

	/*
	多线程压测：
	多个生产者、多个消费者同时操作栈，最终比较所有弹出值的总和与理论总和的差。
	若相等，说明无数据丢失 / 重复，且无 ABA。
	同时可用 - fsanitize = thread 编译检测数据竞争。
	*/

	HazardQueue stack;
	std::atomic<long long> total_pushed = 0;
	std::atomic<long long> total_poped = 0;



	auto producer = [&](int start) {
		//std::cout << "Producer started with start=" << start << std::endl;
		for (int i = 0; i < N; ++i)
		{
			stack.enqueue(start + i);
			++total_pushed;
		}
		//std::cout << "Producer finished" << std::endl;
		};

	std::thread consumer([&] {
		//std::cout << "Consumer started" << std::endl;
		int val = 0;
		int tid = 0;
		for (int i = 0; i < N* PRODUCERS; ++i)
		{
			// 判断是否有数据，有数据才能pop成功
			while (!stack.dequeue(val, tid)) { std::this_thread::yield(); }

			total_poped += val;
		}
		//std::cout << "Consumer finished" << std::endl;
	});

	//std::cout << "Creating threads..." << std::endl;
	std::vector<std::thread> vecThreadList;
	for (int i = 0; i < PRODUCERS; ++i)
	{
		vecThreadList.emplace_back(producer, i * N);
		//std::cout << "Producer thread " << i << " created" << std::endl;
	}


	for (auto& th : vecThreadList)
	{
		th.join();
	}

	consumer.join();

	long long expected = (long long)N * PRODUCERS * (N * PRODUCERS - 1) / 2;
	std::cout << "Enqueued count: " << total_pushed << std::endl;
	std::cout << "Dequeued sum  : " << total_poped << std::endl;
	std::cout << "Expected sum  : " << expected << std::endl;

	if (total_poped == expected)
		std::cout << "✅ Test passed (no data race / ABA)." << std::endl;
	else
		std::cout << "❌ Test failed." << std::endl;

	return 0;
}
