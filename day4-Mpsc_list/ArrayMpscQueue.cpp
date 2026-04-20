#include <thread>
#include <iostream>
#include <atomic>
#include <vector>
#include <chrono>
#include <mutex>


template<typename T>

// 用版本号来替代 true/false  的状态值避免数据回绕
class ArrayMpscQueue
{
private:
	struct Slot{
		std::atomic<uint64_t> version_no = 0;
		T data;
	};

	std::vector<Slot> slots;
	std::atomic<uint64_t> tail = 0;
	std::atomic<uint64_t> head = 0;
	uint64_t capacity;

public:
	ArrayMpscQueue(size_t cap):capacity(cap), slots(cap) {}

	// 多线程安全
	bool push(const T& value)
	{
		uint64_t tail_value;
		uint64_t index;
		uint64_t expected_version;

		do{
			tail_value = tail.load(std::memory_order_acquire);
			index = tail_value % capacity;
			expected_version = (tail_value / capacity) * 2;

			if (slots[index].version_no.load(std::memory_order_acquire) != expected_version)
			{
				return false;
			}

		} while(!tail.compare_exchange_weak(tail_value, tail_value+1, 
			std::memory_order_acq_rel, std::memory_order_acquire));


		slots[index].data = value;
		slots[index].version_no.store(expected_version + 1, std::memory_order_release);
		return true;
	}

	// 单线程执行版本
	bool pop(T& value)
	{
		// 是否为空队列
		uint64_t head_value = head.load(std::memory_order_acquire);
		uint64_t index = head_value % capacity;
		uint64_t expected_version = (head_value / capacity) * 2;

		if(expected_version+1 != slots[index].version_no.load(std::memory_order_acquire))
		{
			return false;
		}

		value = slots[index].data;
		slots[index].version_no.store(expected_version + 2, std::memory_order_release);
		head.store(head_value+1, std::memory_order_release);
		return true;
	}
};



int main()
{
	ArrayMpscQueue<int> arrayQueue(10);
	const int MaxNum = 10;

	auto producer = [&](int nStart) {
		for (int i = 0; i < MaxNum; ++i)
		{
			int value = i + nStart;
			while(!arrayQueue.push(value))
			{
				std::this_thread::yield();
			}
			
		}
	};


	int nSum = 0;
	auto consume = [&]() {
		int nValue = 0;
		for (int index = 0; index < 3 * MaxNum; )
		{
			while(!arrayQueue.pop(nValue))
			{
				std::this_thread::yield();
			}


			nSum += nValue;
			++index;
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


