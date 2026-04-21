markdown

# Day1： 生产者-消费者 

## 核心收获
-- 1. 通过使用 std::mutex 来对线程间的共享变量 std::mutex 进行加锁和解锁。

-- 2.使用std:lock_guard 会自动构造的时候加锁，析构的时候解锁。 这样能更好的对临界区代码进行保护。

-- 3.创建完线程需要 显式的调用 join 或者detach

## 代码
-- producer_consumer_queue.cpp

## 测试
-- 一切正常。
