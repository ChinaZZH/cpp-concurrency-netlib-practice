#include <iostream>
#include <thread>
#include <vector>
#include <shared_mutex>
#include <chrono>

class ThreadSafeCounter
{
private:
    mutable std::shared_mutex m_share_mtx;
    int m_value = 0;

public:
    // 读锁
    int get() const {
        std::shared_lock<std::shared_mutex> share_lk(m_share_mtx);
        return m_value;
    }

    // 写锁
    void increament()
    {
        std::unique_lock<std::shared_mutex> share_lk(m_share_mtx);
        ++m_value;
    }

    // 批量写操作
    void add(int delta)
    {
        std::unique_lock<std::shared_mutex> share_lk(m_share_mtx);
        m_value += delta;
        std::this_thread::sleep_for(std::chrono::microseconds(10));
    }
};


int main()
{
    ThreadSafeCounter safeCounter;
    const int READERS = 10;
    const int WRITERS = 2;
    const int ITERATIONS = 100;

    std::vector<std::thread> threads;

    // 创建读线程
    for(int i = 0; i < READERS; ++i)
    {
        threads.emplace_back([&safeCounter, i]{
            for(int j = 0; j < ITERATIONS; ++j)
            {
                int val = safeCounter.get();
                std::cout << "Reader " << i << " sees " << val << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }

    // 创建写线程
    for(int i = 0; i < WRITERS; ++i)
    {
        threads.emplace_back([&safeCounter, i] {
            for (int j = 0; j < ITERATIONS; ++j)
            {
                safeCounter.increament();
                std::cout << "Writer " << i << " increments to " << safeCounter.get() << std::endl;
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
    }

    for(auto& t: threads)
    {
        t.join();
    }

    std::cout << "Final value: " << safeCounter.get() << std::endl;
    return 0;
}



