#include <thread>
#include <iostream>
#include <atomic>
#include <vector>
#include <chrono>
#include <mutex>


template<typename T>

// 基于cas的mpcs队列
class ListMpscQueue
{
private:
	struct Node {
		T data;
		std::atomic<Node*> next;
		
		Node(const T& value) : data(value), next(nullptr) {} 
	};

	std::atomic<Node*> head;
	std::atomic<Node*> tail;

public:
	ListMpscQueue()
	{
		// 默认有头结点，避免head和tail节点为nullptr,就是head,tail一定有数值,
		// 则避免了判断head和tail是否为空
		Node* dummy = new Node(T());
		head.store(dummy, std::memory_order_relaxed);
		tail.store(dummy, std::memory_order_relaxed);
	}

	~ListMpscQueue()
	{
		// 清理所有节点
		while(head.load() != nullptr)
		{
			Node* head_node = head.load();
			head.store(head->next.load());
			delete head_node;
		}
	}

	// 多线程安全
	// 创建虚拟节点，让push只要负责tail相关节点即可。
	void push(const T& value)
	{
		Node* new_node = new Node(value);
		Node* old_tail = tail.exchange(new_node, std::memory_order_acq_rel);
		old_tail->next.store(new_node, std::memory_order_release);
	}

	// 单线程执行版本
	// 创建虚拟节点，让pop只要负责head节点即可
	bool pop(T& value)
	{
		Node* old_head = head.load(std::memory_order_acquire);
		Node* new_head = old_head->next.load(std::memory_order_acquire);
		if(new_head == nullptr)
		{
			return false;
		}

		value = new_head->data;
		head.store(new_head, memory_order_release);
		delete old_head;
		return true;
	}
};



int main()
{
	ListMpscQueue<int> arrayQueue;
	const int MaxNum = 10;

	auto producer = [&](int nStart) {
		for (int i = 0; i < MaxNum; ++i)
		{
			int value = i + nStart;
			arrayQueue.push(value);
		}
		};


	int nSum = 0;
	auto consume = [&]() {
		int nValue = 0;
		for (int index = 0; index < 3 * MaxNum; )
		{
			if (arrayQueue.pop(nValue))
			{
				nSum += nValue;
				++index;
			}
		}
		};


	std::thread pro_t1(producer, 0);
	std::thread pro_t2(producer, MaxNum);
	std::thread pro_t3(producer, MaxNum + MaxNum);

	std::thread consume_t(consume);

	pro_t1.join();
	pro_t2.join();
	pro_t3.join();
	consume_t.join();

	std::cout << "Sum:" << nSum << std::endl;
	return 0;
}



