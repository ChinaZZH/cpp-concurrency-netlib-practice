markdown

# Day76： 极致性能优化 无锁队列替换成互斥锁

## 核心收获

-- 1.  通过用无锁队列替换互斥锁，减少因为锁竞争而带来的性能消耗和效率延迟， 提高性能。

-- 2. 当前环境为event_loop 的io线程为6个线程, 线程池的线程个数为12个 ，就是cpu核心数目。


## 结论
完整性能对比（客户端100 线程，每个线程10000 请求/线程，总 100 万请求）
| 序号 |EventLoop 队列|	任务线程池队列|	同步 QPS|	异步 QPS|	同步 P99 (μs)|	异步 P99 (μs)|	备注|
|------|-----------------|------------------|------------|------------|------------------|---------------|------|
|1	|互斥锁	|互斥锁	|41,775|	262,833	|5,205	|2,986,281	|基线|
|2	|互斥锁	|阻塞无锁队列|	104,611|	303,556|	4,080|	2,789,906	|最佳同步|
|3	|互斥锁	|非阻塞无锁队列|	87,636|	262,034|	5,409|	2,773,010	|接近 #2|
|4	|无锁队列|	阻塞无锁队列|	73,087|	217,134|	4,669|	3,925,732	|性能下降|
|5	|无锁队列|	非阻塞无锁队列|	80,488|	219,829|	4,690|	3,632,602|	性能下降|
|6|	无锁队列|	互斥锁|	❌ 大量超时|	❌ 大量超时|	—|	—|	不可用|

-- 0. 最佳配置
EventLoop 使用互斥锁 + 任务线程池使用阻塞无锁队列

同步 QPS：104,611（比基线提升 150%）

异步 QPS：303,556（比基线提升 15.5%）

同步 P99 延迟：4,080 μs（比基线降低 22%）

无超时，稳定性最佳。

-- 1. 为什么 EventLoop 用无锁队列反而更差？

    EventLoop 的 pendingFunctors_ 是单消费者（IO线程）多生产者（工作线程）。
 
    无锁队列（ConcurrentQueue）虽然避免了锁，但引入了昂贵的原子操作和内存序，当任务量很大时（每秒几十万次），其开销可能超过互斥锁。
 
    互斥锁在低竞争下非常快：IO线程几乎独占锁，工作线程仅短暂持有，因此互斥锁版本性能反而更好。

-- 2. 为什么 EventLoop 无锁 + 任务池互斥锁会导致大量超时？

   任务池互斥锁成为系统瓶颈：工作线程在提交任务到线程池时，需要竞争全局互斥锁，导致大量线程阻塞。任务无法及时处理，累积在队列中，客户端等待超时。

   无锁 EventLoop 放大了问题：即使 IO 线程能快速处理，任务池的锁依然阻塞了大部分请求，最终导致超时。

   致命组合：EventLoop 无锁 + 任务池互斥锁 → 大量超时，系统不可用。这印证了“锁竞争传递”效应：一个地方的锁会成为整个系统瓶颈。

-- 3. 为什么任务线程池用阻塞无锁队列最好？
   
   BlockingConcurrentQueue 内部使用无锁算法，且当队列满时阻塞生产者（可选），避免了忙等和锁竞争。相比互斥锁，它能让工作线程更平滑地提交任务，大幅提升吞吐。


## 性能优化结论：

-- 1. 无锁队列在高并发提交场景下优势明显，但并非所有场景都适用。

-- 2. EventLoop 的跨线程任务队列因竞争度低，互斥锁性能更优。

-- 3. 任务线程池应使用 BlockingConcurrentQueue（阻塞无锁队列），可获得最佳吞吐和延迟。

-- 4. 避免混用：EventLoop 无锁 + 任务池互斥锁会导致系统崩溃。

## 测试
-- 一切正常。


## 具体测试性能数据

-- 1.  客户端异步调用 AsyncCall. 100线程。   

     event_loop用互斥锁mutex  任务线程池用阻塞性无锁队列 性能远远高于 event_loop 和任务线程池 都是用互斥锁 mutex

```cpp
event_loop 和任务线程池 都是用互斥锁 mutex
start Async Call 100 thread and req_per_threads 10000:
CallAsync  Total request: 1000000
Success: 1000000 , Timeout: 0 , Fail:0
Total time(second): 3.8047
QPS: 262833
Average latency: 1.86384e+06
P50 latency: 1965785
P90 latency: 2567286
P99 latency: 2986281
P999 latency: 3117479

start Call 100 thread and req_per_threads 10000:
Call  Total request: 1000000
Success: 1000000 , Timeout: 0 , Fail:0
Total time(second): 23.9376
QPS: 41775.3
Average latency: 2348.94
P50 latency: 2224
P90 latency: 3622
P99 latency: 5205
P999 latency: 7660


event_loop用互斥锁mutex  任务线程池用阻塞性无锁队列
start Async Call 100 thread and req_per_threads 10000:
CallAsync  Total request: 1000000
Success: 1000000 , Timeout: 0 , Fail:0
Total time(second): 3.29429
QPS: 303556
Average latency: 1.297e+06
P50 latency: 1202972
P90 latency: 2213086
P99 latency: 2789906
P999 latency: 2898145


start Call 100 thread and req_per_threads 10000:
Call  Total request: 1000000
Success: 1000000 , Timeout: 0 , Fail:0
Total time(second): 9.55926
QPS: 104611
Average latency: 893.115
P50 latency: 681
P90 latency: 1777
P99 latency: 4080
P999 latency: 7197


```

-- 2.  客户端异步调用 AsyncCall. 100线程。   

     event_loop用互斥锁mutex  任务线程池用阻塞性无锁队列 性能远远高于 event_loop为无锁队列， 线程池为阻塞型无锁队列
```cpp
event_loop用互斥锁mutex  任务线程池用阻塞性无锁队列
start Async Call 100 thread and req_per_threads 10000:
CallAsync  Total request: 1000000
Success: 1000000 , Timeout: 0 , Fail:0
Total time(second): 3.29429
QPS: 303556
Average latency: 1.297e+06
P50 latency: 1202972
P90 latency: 2213086
P99 latency: 2789906
P999 latency: 2898145


start Call 100 thread and req_per_threads 10000:
Call  Total request: 1000000
Success: 1000000 , Timeout: 0 , Fail:0
Total time(second): 9.55926
QPS: 104611
Average latency: 893.115
P50 latency: 681
P90 latency: 1777
P99 latency: 4080
P999 latency: 7197

## event_loop为无锁队列， 线程池为阻塞型无锁队列
 start Async Call 100 thread and req_per_threads 10000:
CallAsync  Total request: 1000000
Success: 1000000 , Timeout: 0 , Fail:0
Total time(second): 4.60545
QPS: 217134
Average latency: 1.94272e+06
P50 latency: 1934546
P90 latency: 3129878
P99 latency: 3925732
P999 latency: 4264888


start Call 100 thread and req_per_threads 10000:
Call  Total request: 1000000
Success: 1000000 , Timeout: 0 , Fail:0
Total time(second): 13.6822
QPS: 73087.6
Average latency: 1279.98
P50 latency: 1055
P90 latency: 2355
P99 latency: 4669
P999 latency: 7643


## event_loop为无锁队列， 线程池为无阻塞无锁队列
start Async Call 100 thread and req_per_threads 10000:
CallAsync  Total request: 1000000
Success: 1000000 , Timeout: 0 , Fail:0
Total time(second): 4.54899
QPS: 219829
Average latency: 1.71326e+06
P50 latency: 1688370
P90 latency: 2736906
P99 latency: 3632602
P999 latency: 4008267


start Call 100 thread and req_per_threads 10000:
Call  Total request: 1000000
Success: 1000000 , Timeout: 0 , Fail:0
Total time(second): 12.4241
QPS: 80488.8
Average latency: 1094.09
P50 latency: 893
P90 latency: 1923
P99 latency: 4690
P999 latency: 11639
```

-- 有当event_loop为无锁队列，但是任务线程池为互斥锁的时候客户端会出现大量的超时
