
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

	Node(int id) :data(id), next(nullptr) {}
};


// 全局风险指针(每个消费者一个)
constexpr int MAX_THREADS = 128;
std::atomic<Node*> hazard_ptrs[MAX_THREADS];
std::atomic<int> thread_counter = 0;
thread_local int thread_id = -1;


void register_thread()
{
	if(-1 == thread_id)
	{
		thread_id = thread_counter.fetch_add(1, std::memory_order_relaxed);
		hazard_ptrs[thread_id].store(nullptr, std::memory_order_relaxed);
	}
}


void set_hazard(Node* node)
{
	hazard_ptrs[thread_id].store(node, std::memory_order_release);
}


void clear_hazard()
{
	hazard_ptrs[thread_id].store(nullptr, std::memory_order_release);
}

// 待回收链表
std::atomic<Node*>  retired_list = nullptr;

// 将节点加入待回收链表
void retire(Node* node)
{
	if (!node)
	{
		return;
	}

	Node* old_head = retired_list.load(std::memory_order_relaxed);
	do {
		node->next.store(old_head, std::memory_order_relaxed);
	} while (retired_list.compare_exchange_weak(old_head,
		node,
		std::memory_order_release,
		std::memory_order_relaxed)
		);
}


// 回收安全节点（扫描所有的风险指针）
void scan_and_reclaim()
{
	Node* list = retired_list.exchange(nullptr, std::memory_order_acquire);
	if (!list)
	{
		// 没有待回收节点
		return;
	}

	Node* hazards[MAX_THREADS];
	int count = 0;
	int total = thread_counter.load(std::memory_order_acquire);
	for (int i = 0; i < total; ++i)
	{
		Node* ptr = hazard_ptrs[i].load(std::memory_order_acquire);
		if (ptr)
		{
			hazards[count] = ptr;
			++count;
		}
	}

	Node* it = list;
	while (it)
	{
		Node* next_node = (it->next).load(std::memory_order_acquire);
		bool bSafe = true;
		for (int i = 0; i < count; ++i)
		{
			if (hazards[i] == it)
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

public:
	// 创建虚拟节点
	HazardQueue() :head_(nullptr), tail_(nullptr)
	{
		Node* dummy = new Node(0);
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
		register_thread();
		Node* old_head = head_.load(std::memory_order_acquire);
		while (true)
		{
			// 队列为空，只有dummy节点
			if (old_head == tail_.load(std::memory_order_acquire))
			{
				return false;
			}

			Node* node = old_head->next.load(std::memory_order_acquire);
			if (!node)
			{
				return false;
			}

			// 设置风险指针为操作节点
			set_hazard(old_head);

			// 二次验证
			if (head_.load(std::memory_order_acquire) != old_head)
			{
				clear_hazard();
				old_head = head_.load(std::memory_order_acquire);
				continue;
			}

			

			if (head_.compare_exchange_weak(old_head, node, std::memory_order_release, std::memory_order_relaxed))
			{
				val = node->data;
				clear_hazard();

				retire(old_head);
				scan_and_reclaim();
				return true;
			}

			// cas失败
			clear_hazard();
		}

		return false;
	}
};



int main()
{
	
	const int N = 100;			// 每个线程的操作次数
	const int PRODUCERS = 4;	// 生产者线程数
	const int CONSUMERS = 4;	// 消费者线程数
	// 
	// 多线程压测

	/*
	多线程压测：
	多个生产者、多个消费者同时操作栈，最终比较所有弹出值的总和与理论总和的差。
	若相等，说明无数据丢失 / 重复，且无 ABA。
	同时可用 - fsanitize = thread 编译检测数据竞争。
	*/

	HazardQueue q;
	std::atomic<long long> total_enqueued = 0;
	std::atomic<long long> total_dequeued = 0;
	std::atomic<int> dequeue_count = 0;


	auto producer = [&](int start) {
		register_thread();
		for (int i = 0; i < N; ++i)
		{
			q.enqueue(start + i);
			++total_enqueued;
		}
	};

	auto consumer = [&] {
		register_thread();
		int val = 0;
		int tid = 0;
		int forTotal = N * PRODUCERS / CONSUMERS;
		for (int i = 0; i < forTotal; ++i)
		{
			// 判断是否有数据，有数据才能pop成功
			while (!q.dequeue(val, tid)) { std::this_thread::yield(); }

			total_dequeued += val;
			++dequeue_count;
		}
	};


	std::vector<std::thread> vecThreadList;
	for (int i = 0; i < PRODUCERS; ++i)
	{
		vecThreadList.emplace_back(producer, i * N);
	}

	for (int i = 0; i < CONSUMERS; ++i)
	{
		vecThreadList.emplace_back(consumer);
	}

	for (auto& th : vecThreadList)
	{
		th.join();
	}


	long long expected = (long long)N * PRODUCERS * (N * PRODUCERS - 1) / 2;
	std::cout << "Enqueued : " << total_enqueued << std::endl;
	std::cout << "Dequeued sum : " << total_dequeued << std::endl;
	std::cout << "Dequeue count: " << dequeue_count << std::endl;
	std::cout << "Expected sum : " << expected << std::endl;

	if (total_dequeued == expected && dequeue_count == PRODUCERS * N)
		std::cout << "✅ Test passed" << std::endl;
	else
		std::cout << "❌ Test failed" << std::endl;

	return 0;
}
