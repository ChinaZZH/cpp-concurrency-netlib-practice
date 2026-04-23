markdown

## Day8： 读写锁 (shared_mutex)
## 核心收获
-- 1.当在写多读少的情况下， 如果是读优先的调度策略下则写操作如果被过长时间阻塞造成写饥饿的情况。如果是写优先或者公平队列调度的情况下则读的并发度会降低。
--   再加上读写锁额外的开销.所以写多读少的情况下他的性能是比互斥锁要更低的std::mutex.

-- 2.使用compare_exchange_weak, 如果当前原子变量的值老是不相等，则有可能导致活锁。就是在很长一段时间内或者说永远都获取不到真正的最新的数值。 所以失败的时候需要使用std::memory_order_acquire 更新其他线程同步过来的数值，以保证能及时更新数值。

-- 3.std::shared_time_mutex 相比与shared_mutex 加了一个超时机制，设置一个等待锁的超时时间避免了无限期等待加锁的情况。只有当你的逻辑确实需要防止线程因无法获取锁
--   而无限期阻塞时，才应该使用它。例如，一个监控线程试图在1秒内获取读锁，如果失败则放弃并记录日志，而不是一直等待。

-- 4. pop的时候，进入while循环其实也改变了head的数值，head是共享变量也需要使用 std::memory_order_release 同步到其他线程。

-- 5. push的时候 可以使用 new_node->next 去替换 head_node. 这样当他cas失败的时候 会先执行 new_node->next = head. 可以简化代码。 不过可以使用代码中的例子2 这样逻辑也可以比较简单。

-- 6. compare_exchange_weak 这个函数中的cas 的动作包含了 读-改-写， 这样是一体的。并且读的默认使用的是 std::memory_order_acquire, 写默认使用的是 std::memory_order_release. 所以 push 中使用 compare_exchange_weak 成功的内存序列是 std::memory_order_release， 就算是这个 读旧值默认的也是 std::memory_order_acquire。

## 代码
-- ThreadSafeCounter.cpp  读写锁使用案例
-- TestReaderFirst.cpp    判断读写锁的调度策略是否是读优先策略
-- TestWriteFirst.cpp     判断读写锁的调度策略是否是写优先策略
-- 如果既不是读优先也不是写优先，那就是公平队列调度策略。

## 测试
-- 一切正常。
