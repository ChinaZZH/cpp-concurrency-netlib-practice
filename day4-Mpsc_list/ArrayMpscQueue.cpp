#include <thread>
#include <iostream>
#include <atomic>
#include <vector>
#include <chrono>
#include <mutex>


template<typename T>

// 多生产者单消费者版本的队列，会造成数据回绕。
class ArrayMpscQueue
{
private:
	struct Slot{
		std::atomic<bool> filled = false;
		T data;
	};

	std::vector<Slot> slots;
	std::atomic<uint64_t> tail = 0;
	uint64_t head = 0;
	uint64_t capacity;

public:
	ArrayMpscQueue(size_t cap):capacity(cap), slots(cap) {}

	// 多线程安全
	bool push(const T& value)
	{
		// 取出下一个索引位置的index
		// {数据回绕问题}
		// 当取到next_index然后该线程挂起的时候，另外的线程将队列填满，再有一个新的线程先有push_T0
		// 线程先将数据push入队列，就会造成数据回绕的问题。
		// 就是后面的数据先入队列，这个T0队列需要等待消费者消费了其中的数据才能再讲数据放入index=0的位置上。
		uint64_t index = tail.fetch_add(1, std::memory_order_acquire);
		index = index % capacity;
		
		// 自旋等待这个next_index为空数据的位置
		while(slots[index].filled.load(std::memory_order_acquire))
		{
			// 队列满，自选等待
			std::this_thread::yield();
			continue;
		}

		slots[index].data = value;
		slots[index].filled.store(true, std::memory_order_release);
		return true;
	}

	// 单线程执行版本
	bool pop(T& value)
	{
		// 是否为空队列
		uint64_t index = head % capacity;
		if(!slots[index].filled.load(std::memory_order_acquire))
		{
			return false;
		}

		value = slots[index].data;
		slots[index].filled.store(false, std::memory_order_release);
		++head;
		return true;
	}
};


int main()
{
	ArrayMpscQueue<int> arrayQueue(5);
	const int MaxNum = 10;

	auto producer = [&](int nStart) {
		for(int i = 0; i < MaxNum; ++i)
		{
			int value = i + nStart;
			arrayQueue.push(value);
		}
	};


	int nSum = 0;
	auto consume = [&]() {
		int nValue = 0;
		for(int index = 0; index < 3*MaxNum; )
		{
			if(arrayQueue.pop(nValue))
			{
				nSum += nValue;
				++index;
			}
		}
	};


	std::thread pro_t1(producer, 0);
	std::thread pro_t2(producer, MaxNum);
	std::thread pro_t3(producer, MaxNum+ MaxNum);

	std::thread consume_t(consume);
	
	pro_t1.join();
	pro_t2.join();
	pro_t3.join();
	consume_t.join();

	std::cout << "Sum:" << nSum << std::endl;
	return 0;
}




