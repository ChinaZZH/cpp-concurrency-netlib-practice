#include <thread>
#include <iostream>
#include <atomic>
#include <vector>

// 通过两种方式实现自旋锁  std::atomic<bool> 和 std::atomic_flag at = ATOMIC_FLAG_INIT;
class SpinLock
{
public:
	std::atomic<bool> locked = false;

public:
	void lock()
	{
		while(locked.exchange(true, std::memory_order_acquire))
		{
			std::this_thread::yield();
		}
	}

	void unlock()
	{
		locked.store(false, std::memory_order_release);
	}
};



class AtomicFlagSpinLock
{
public:
	std::atomic_flag locked = ATOMIC_FLAG_INIT;

public:
	void lock()
	{
		while(locked.test_and_set(std::memory_order_acquire))
		{
			std::this_thread::yield();
		}
	}

	void unlock()
	{
		locked.clear(std::memory_order_release);
	}
};


int main()
{
	{
		SpinLock spin;
		int counter = 0;

		auto task = [&]() {
			for (int i = 0; i < 10000; ++i)
			{
				spin.lock();
				++counter;
				spin.unlock();
			}
			};

		std::vector<std::thread> threads_list;
		for (int i = 0; i < 4; ++i)
		{
			threads_list.emplace_back(task);
		}

		for (auto& th : threads_list)
		{
			th.join();
		}

		std::cout << "Final counter:" << counter << std::endl;
	}
	

	
	{
		// AtomicFlagSpinLock 锁
		AtomicFlagSpinLock spin;
		int counter = 0;

		auto task = [&]() {
			for (int i = 0; i < 10000; ++i)
			{
				spin.lock();
				++counter;
				spin.unlock();
			}
			};

		std::vector<std::thread> threads_list;
		for (int i = 0; i < 4; ++i)
		{
			threads_list.emplace_back(task);
		}

		for (auto& th : threads_list)
		{
			th.join();
		}

		std::cout << "AtomicFlagSpinLock Final counter:" << counter << std::endl;
	}
	


	return 0;
}