markdown

# Day2： 条件变量的学习 

## 核心收获
-- 1. 通过使用 std::std::condition_variable 条件变量更好的利用多线程来提高并发性。 多线程的使用的应用场景其实也和多协程式类似的，主要就是惰性计算和异步执行逻辑。

-- 2. 通过业务隔离来使用不同线程间的相互配合，其中在游戏场景中最明显的惰性计算就是定时器的运用(在指定时间完成某项任务)，异步执行主要就是各种IO (网络IO，数据库IO，文件IO).

-- 3. std::unique_lock 相比与 std::mutex_lock的优点就是他可以任务的控制加锁和解锁，同时可以搭配条件变量使用。

-- 4. std::unique_lock 在wait的使用如果条件不满足唤醒，则会先解锁，把mutex互斥锁给其他线程使用。当条件满足唤醒他的时候  则会重新加锁。

-- 5. std::unique_lock 和 std::mutex 搭配起来使用。 同时 std::unique_lock 中的 wait_x 相关函数 也和 notify_x相关函数搭配起来使用。

## 代码
-- conditon_variable.cpp

## 测试
-- 一切正常。
