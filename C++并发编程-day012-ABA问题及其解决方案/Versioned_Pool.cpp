
#include <atomic>
#include <iostream>
#include <thread>
#include <vector>
#include <cassert>

template<typename T, size_t N>
class VersionedPool
{
	static_assert(N > 0 && (N & N-1) == 0, "N must be power of 2");

	struct Slot
	{
		std::atomic<uint64_t> version; // 低1位: 偶数0=空闲  奇数1=占用 防止ABA问题
		T data;

		Slot() :data(0) {}
	};

	Slot slots_[N];
	std::atomic<size_t> next_ = 0; // 用于负债均衡的其实扫描索引

public:
	VersionedPool()
	{
		for(int i = 0; i < N; ++i)
		{
			slots_[i].version.store(i * 2, std::memory_order_relaxed);
		}
	}

	//分配一个对象，返回指针;如果没有空闲槽位返回nullptr
	T* allocate()
	{
		// next+1同时返回旧值 先占用next_值槽位
		size_t start_index = next_.fetch_add(1, std::memory_order_relaxed);
		
		// 循环遍历找个空的槽位
		for(int i = 0; i < N; ++i)
		{
			size_t index = (start_index)+i;
			index = index & (N - 1);

			// 奇数已经被占用
			uint64_t old_versioned = slots_[index].version.load(std::memory_order_acquire);
			if(1 == (old_versioned & 0x01))
			{
				continue;
			}

			uint64_t new_versioned = old_versioned + 1; // 测试用 =1  可以产生ABA问题，版本号不递增，
			if(slots_[index].version.compare_exchange_strong(old_versioned,
				new_versioned,
				std::memory_order_release,
				std::memory_order_relaxed))
			{
				return &(slots_[index].data);
			}
		}

		// 已经满了，没有空闲槽位了
		return nullptr;
	}

	// 释放对象(通过指针)
	bool deallocate(T* ptr)
	{
		if(!ptr)
		{
			return false;
		}

		int index = ptr - (&(slots_[0].data));
		uint64_t old_versioned = slots_[index].version.load(std::memory_order_acquire);
		if (0 == (old_versioned & 0x01))
		{
			// 重大问题，无法规范(这个slot的状态是空闲，说明就没有被占用过)
			return false;
		}

		uint64_t new_versioned = old_versioned + 1; // 测试用=0替代 可以产生ABA问题，版本号不递增
		//slots_.version.store(new_versioned, std::memory_order_release);
		if(!slots_[index].version.compare_exchange_strong(old_versioned,
			new_versioned,
			std::memory_order_release,
			std::memory_order_relaxed))
		{
			return false;
		}

		return true;
	}

	// 获取内存池的大小(调试用)
	constexpr size_t size() const { return N; }
};

// 测试对象
struct TestObj
{
	int id;
	double value;
};


int main()
{
	constexpr size_t POOL_SIZE = 64;
	VersionedPool<TestObj, POOL_SIZE> pool;

	const int THREADS = 8;
	const int ITERS = 100000;

	std::atomic<int>  total_alloc = 0;
	std::atomic<int>  total_dealloc = 0;
	std::atomic<int>  alloc_fail = 0;

	auto worker = [&](int start) {
		for(int i = 0; i < ITERS; ++i)
		{
			TestObj* obj = pool.allocate();
			if(obj)
			{
				obj->id = start+i;
				obj->value = (start+i) * 0.1f;
				++total_alloc;
				bool bCheck = pool.deallocate(obj);
				if (bCheck)
				{
					++total_dealloc;
				}
				
			}
			else {
				++alloc_fail;
				std::this_thread::yield();
			}
		}
	};


	std::vector<std::thread> threads;
	for (int i = 0; i < THREADS; ++i) {
		threads.emplace_back(worker, i* ITERS);
	}

	for (auto& th : threads) {
		th.join();
	}

	std::cout << "Total allocate success: " << total_alloc << std::endl;
	std::cout << "Total deallocate       : " << total_dealloc << std::endl;
	std::cout << "Alloc failures (pool full): " << alloc_fail << std::endl;

	// 校验：成功分配的次数应等于释放的次数
	if (total_alloc == total_dealloc) {
		std::cout << "✅ Allocation and deallocation counts match." << std::endl;
	}
	else {
		std::cout << "❌ Counts mismatch!" << std::endl;
	}

	// 额外校验：由于池容量有限，总分配成功次数不应超过 THREADS * ITERS，且应大致等于池容量 * 轮转次数
	// 但主要验证无竞争和 ABA
	return 0;
}

