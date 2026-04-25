
#include <iostream>
#include <atomic>
#include <thread>
#include <chrono>
#include <vector>

struct Node
{
	int data;
	Node* next;

	Node(int val) :data(val), next(nullptr) {}
};


// 固定大小的对象池，保证地址可以重用(内存池)
class ObjectPool
{
public:
	explicit ObjectPool(size_t size)
	{
		pool.reserve(size);
		for(int i = 0; i < size; ++i)
		{
			pool.emplace_back(new Node(0));
			if(0 != i)
			{
				pool[i - 1]->next = pool[i];
			}
			else {
				freeList = pool[0];
			}
		}
	}

	~ObjectPool()
	{
		for(int i = 0; i < pool.size(); ++i)
		{
			delete pool[i];
		}

		freeList = nullptr;
	}


	Node* allocate(int value)
	{
		if(!freeList)
		{
			return nullptr;
		}

		Node* node = freeList;
		freeList = node->next;

		node->data = value;
		node->next = nullptr;
		return node;
	}

	void deallocate(Node* node)
	{
		if (!node)
		{
			return;
		}

		node->next = freeList;
		freeList = node;
	}

private:
	std::vector<Node*> pool;
	Node* freeList;
};


// 全局变量器
std::atomic<Node*>  head{ nullptr };
ObjectPool pool(10);

void thread_1_function()
{
	Node* old_head = head.load(std::memory_order_acquire);
	// 让线程2有机会介入
	std::this_thread::sleep_for(std::chrono::milliseconds(10));

	if(head.compare_exchange_strong(old_head, 
		old_head->next, 
		std::memory_order_release, 
		std::memory_order_relaxed))
	{
		std::cout << "Thread1: popped node at " << old_head
			<< " with data " << old_head->data << std::endl;
		// 注意：这里本应释放节点，但由于演示ABA，我们暂时不释放给对象池，
		// 否则线程2的重用地址可能受影响。为了清晰，我们让线程1不归还节点，
		// 但实际生产环境必须归还。这里仅用于演示 ABA 原理。
	}
	else {
		std::cout << "Thread1: CAS failed" << std::endl;
	}
}


void thread_2_function()
{
	Node* old_head = head.load(std::memory_order_acquire);
	if (old_head && head.compare_exchange_strong(old_head,
		old_head->next,
		std::memory_order_release,
		std::memory_order_relaxed))
	{
		std::cout << "Thread2: popped node at " << old_head
			<< " with data " << old_head->data << std::endl;

		// 将旧节点归还对象池（地址可重用）
		pool.deallocate(old_head);

		// 分配一个新节点（很可能获得刚刚释放的地址）
		Node* new_node = pool.allocate(999);
		std::cout << "Thread2: allocated new node at " << new_node
			<< " with data " << new_node->data << std::endl;

		// cas重试 将新节点压回栈
		new_node->next = head.load(std::memory_order_relaxed);
		while(head.compare_exchange_weak(new_node->next, 
			new_node,
			std::memory_order_release,
			std::memory_order_relaxed))
		{
			// 自旋直到成功
			std::cout << "Thread2: pushed new node, head=" << head.load() << std::endl;
		}
	}
}


int main()
{
	Node* init = pool.allocate(42);
	head.store(init, std::memory_order_relaxed);
	std::cout << "Initial head: " << init << "with data 42" << std::endl;

	std::thread t1(thread_1_function);
	std::thread t2(thread_2_function);
	t1.join();
	t2.join();

	return 0;
}

