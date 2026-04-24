// benchmark.cpp
#include <iostream>
#include <thread>
#include <vector>
#include <future>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <atomic>

// ---------- 1️ MPMC 无锁队列（Day9）----------
template<typename T>
class MPMCQueue {
public:
    explicit MPMCQueue(size_t capacity)
        : capacity_(nextPowerOf2(capacity))
        , mask_(capacity_ - 1)
        , slots_(capacity_)
        , enqueue_pos_(0)
        , dequeue_pos_(0) {
        for (size_t i = 0; i < capacity_; ++i)
            slots_[i].seq.store(i, std::memory_order_relaxed);
    }

    bool enqueue(const T& data) {
        uint64_t pos = enqueue_pos_.load(std::memory_order_relaxed);
        while (true) {
            size_t idx = pos & mask_;
            uint64_t seq = slots_[idx].seq.load(std::memory_order_acquire);
            int64_t diff = static_cast<int64_t>(seq) - static_cast<int64_t>(pos);
            if (diff == 0) {
                if (enqueue_pos_.compare_exchange_weak(pos, pos + 1,
                    std::memory_order_release, std::memory_order_relaxed)) {
                    slots_[idx].data = data;
                    slots_[idx].seq.store(pos + 1, std::memory_order_release);
                    return true;
                }
            }
            else if (diff < 0) {
                return false;
            }
            else {
                pos = enqueue_pos_.load(std::memory_order_relaxed);
            }
        }
    }

    bool dequeue(T& data) {
        uint64_t pos = dequeue_pos_.load(std::memory_order_relaxed);
        while (true) {
            size_t idx = pos & mask_;
            uint64_t seq = slots_[idx].seq.load(std::memory_order_acquire);
            int64_t diff = static_cast<int64_t>(seq) - static_cast<int64_t>(pos + 1);
            if (diff == 0) {
                if (dequeue_pos_.compare_exchange_weak(pos, pos + 1,
                    std::memory_order_release, std::memory_order_relaxed)) {
                    data = slots_[idx].data;
                    slots_[idx].seq.store(pos + capacity_, std::memory_order_release);
                    return true;
                }
            }
            else if (diff < 0) {
                return false;
            }
            else {
                pos = dequeue_pos_.load(std::memory_order_relaxed);
            }
        }
    }

private:
    static size_t nextPowerOf2(size_t n) {
        size_t pow = 1;
        while (pow < n) pow <<= 1;
        return pow;
    }
    struct Slot {
        std::atomic<uint64_t> seq;
        T data;
    };
    const size_t capacity_, mask_;
    std::vector<Slot> slots_;
    std::atomic<uint64_t> enqueue_pos_, dequeue_pos_;
};

// ---------- 2️ 互斥锁线程池（Day6）----------
class MutexThreadPool {
public:
    MutexThreadPool(size_t numThreads) : stop_(false) {
        for (size_t i = 0; i < numThreads; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    {
                        std::unique_lock<std::mutex> lock(mtx_);
                        cv_.wait(lock, [this] { return stop_ || !tasks_.empty(); });
                        if (stop_ && tasks_.empty()) break;
                        task = std::move(tasks_.front());
                        tasks_.pop();
                    }
                    task();
                }
                });
        }
    }
    ~MutexThreadPool() {
        {
            std::lock_guard<std::mutex> lock(mtx_);
            stop_ = true;
        }
        cv_.notify_all();
        for (auto& t : workers_) t.join();
    }
    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::invoke_result_t<F, Args...>> {
        using return_type = std::invoke_result_t<F, Args...>;
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::future<return_type> res = task->get_future();
        {
            std::lock_guard<std::mutex> lock(mtx_);
            tasks_.emplace([task]() { (*task)(); });
        }
        cv_.notify_one();
        return res;
    }
private:
    std::vector<std::thread> workers_;
    std::queue<std::function<void()>> tasks_;
    std::mutex mtx_;
    std::condition_variable cv_;
    bool stop_;
};

// ---------- 3️ 无锁队列线程池（Day9）----------
class LockfreeThreadPool {
public:
    LockfreeThreadPool(size_t numThreads, size_t queueCap = 16384)
        : tasks_(queueCap), stop_(false) {
        for (size_t i = 0; i < numThreads; ++i) {
            workers_.emplace_back([this] {
                while (true) {
                    std::function<void()> task;
                    if (tasks_.dequeue(task)) task();
                    else if (stop_) break;
                    else std::this_thread::yield();
                }
                });
        }
    }
    ~LockfreeThreadPool() {
        stop_ = true;
        for (auto& t : workers_) t.join();
    }
    template<typename F, typename... Args>
    auto enqueue(F&& f, Args&&... args) -> std::future<typename std::invoke_result_t<F, Args...>> {
        using return_type = std::invoke_result_t<F, Args...>;
        auto task = std::make_shared<std::packaged_task<return_type()>>(
            std::bind(std::forward<F>(f), std::forward<Args>(args)...));
        std::future<return_type> res = task->get_future();
        while (!tasks_.enqueue([task]() { (*task)(); })) {
            std::this_thread::yield();
        }
        return res;
    }
private:
    MPMCQueue<std::function<void()>> tasks_;
    std::vector<std::thread> workers_;
    std::atomic<bool> stop_;
};

// ---------- 4️ 基准：单生产者单消费者 mutex 队列 ----------
double baselineTest(int taskCount, int iterPerTask = 10) {
    std::queue<std::function<int()>> tasks;
    std::mutex mtx;
    std::condition_variable cv;
    std::atomic<bool> done{ false };
    std::vector<int> results;
    results.reserve(taskCount);

    std::thread consumer([&] {
        while (true) {
            std::function<int()> task;
            {
                std::unique_lock<std::mutex> lock(mtx);
                cv.wait(lock, [&] { return !tasks.empty() || done; });
                if (tasks.empty() && done) break;
                task = std::move(tasks.front());
                tasks.pop();
            }
            results.push_back(task());
        }
    });


    // 当前时间戳 纳秒级
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < taskCount; ++i) {
        {
            std::lock_guard<std::mutex> lock(mtx);
            tasks.emplace([i, iterPerTask] { 
                int sum = 0;
                for (int j = 0; j < iterPerTask; ++j)
                {
                    sum += i * i;
                }

                return sum;
            });
        }
        cv.notify_one();
    }
    {
        std::lock_guard<std::mutex> lock(mtx);
        done = true;
    }
    cv.notify_all();
    consumer.join();
    // 当前时间戳 纳秒级
    auto end = std::chrono::high_resolution_clock::now();
    double sec = std::chrono::duration<double>(end - start).count();
    return taskCount / sec;
}

// ---------- 通用测试函数 ----------
template<typename Pool>
double runTest(Pool& pool, int taskCount, int iterPerTask = 10) {
    std::vector<std::future<int>> futures;
    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < taskCount; ++i) {
        futures.emplace_back(pool.enqueue([i, iterPerTask] {
            int sum = 0;
            for (int j = 0; j < iterPerTask; ++j) sum += i * i;
            return sum;
            }));
    }
    long long total = 0;
    for (auto& f : futures) total += f.get();
    auto end = std::chrono::high_resolution_clock::now();
    double sec = std::chrono::duration<double>(end - start).count();
    return taskCount / sec;
}

int main() {
    const int TASKS = 500000;
    const int THREADS = 12;
    const int ITER = 10000;
    std::cout << "=== 性能对比测试 ===" << std::endl;
    std::cout << "任务数: " << TASKS << ", 线程池线程数: " << THREADS << ", 每任务循环: " << ITER << std::endl;
    double qps_base = baselineTest(TASKS, ITER);
    double qps_mutex, qps_lockfree;
    {
        MutexThreadPool pool(THREADS);
        qps_mutex = runTest(pool, TASKS, ITER);
    }
    {
        LockfreeThreadPool pool(THREADS, 16384);
        qps_lockfree = runTest(pool, TASKS, ITER);
    }
    std::cout << "\n========== 测试结果 ==========" << std::endl;
    std::cout << "Baseline (串行)         : " << static_cast<int>(qps_base) << " tasks/sec" << std::endl;
    std::cout << "MutexThreadPool         : " << static_cast<int>(qps_mutex) << " tasks/sec" << std::endl;
    std::cout << "LockfreeThreadPool      : " << static_cast<int>(qps_lockfree) << " tasks/sec" << std::endl;
    return 0;
}