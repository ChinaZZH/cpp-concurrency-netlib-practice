markdown

# Day76： 极致性能优化 使用内存池

## 核心收获

-- 1. 使用内存池预分配 替代频繁的new和delete带来的性能开销

-- 2.  服务端的定时器 std::priority_queue<TimerNode> timersFunc_;  使用了内存池 但是性能却没有提高。

## 测试数据

-- 1.  客户端异步调用 100线程，每个线程10000个任务对比， QPS和总时间数都领先，各项数据都下降了。

```cpp
没有使用内存池
start Async Call 100 thread and req_per_threads 10000:
CallAsync  Total request: 1000000
Success: 1000000 , Timeout: 0 , Fail:0
Total time(second): 3.14222
QPS: 318246
Average latency: 1.17878e+06
P50 latency: 1168140
P90 latency: 1873049
P99 latency: 2401310
P999 latency: 2620453

使用了内存池的
start Async Call 100 thread and req_per_threads 10000:
CallAsync  Total request: 1000000
Success: 1000000 , Timeout: 0 , Fail:0
Total time(second): 4.16663
QPS: 240002
Average latency: 1.84728e+06
P50 latency: 1900473
P90 latency: 2749582
P99 latency: 3492017
P999 latency: 3942382

```
-- 2.  客户端同步调用 100线程，每个线程10000个任务对比， QPS和总时间数都领先，各项数据都下降了。

```cpp
没有使用内存池
start Call 100 thread and req_per_threads 10000:
Call  Total request: 1000000
Success: 1000000 , Timeout: 0 , Fail:0
Total time(second): 9.28925
QPS: 107651
Average latency: 861.623
P50 latency: 696
P90 latency: 1619
P99 latency: 3418
P999 latency: 6747


使用了内存池的
start Call 100 thread and req_per_threads 10000:
Call  Total request: 1000000
Success: 1000000 , Timeout: 0 , Fail:0
Total time(second): 13.485
QPS: 74156.5
Average latency: 1283.74
P50 latency: 956
P90 latency: 2494
P99 latency: 6123
P999 latency: 13195
```

-- 3.  客户端异步调用 50线程，每个线程10000个任务对比， QPS和总时间数都领先，各项数据都下降了。

```cpp
没有使用内存池
start Async Call 50 thread and req_per_threads 10000:
CallAsync  Total request: 500000
Success: 500000 , Timeout: 0 , Fail:0
Total time(second): 1.68028
QPS: 297570
Average latency: 852709
P50 latency: 855159
P90 latency: 1218859
P99 latency: 1459440
P999 latency: 1601968

使用了内存池的
start Async Call 50 thread and req_per_threads 10000:
CallAsync  Total request: 500000
Success: 500000 , Timeout: 0 , Fail:0
Total time(second): 2.30709
QPS: 216723
Average latency: 1.14879e+06
P50 latency: 1159114
P90 latency: 1652468
P99 latency: 1991267
P999 latency: 2122572

```


-- 3.  客户端异步调用 50线程，每个线程10000个任务对比， QPS和总时间数都领先，各项数据都下降了。

```cpp
没有使用内存池
start Call 100 thread and req_per_threads 10000:
Call  Total request: 500000
Success: 500000 , Timeout: 0 , Fail:0
Total time(second): 5.41612
QPS: 92317
Average latency: 485.155
P50 latency: 331
P90 latency: 1011
P99 latency: 2443
P999 latency: 4390

使用了内存池的
start Call 100 thread and req_per_threads 10000:
Call  Total request: 500000
Success: 500000 , Timeout: 0 , Fail:0
Total time(second): 7.45994
QPS: 67024.6
Average latency: 697.232
P50 latency: 468
P90 latency: 1454
P99 latency: 3634
P999 latency: 6747

```
## 结论

-- 1. 内存池的引入带来了性能系统的下降，并且增加了系统的复杂性，所以这边不再使用内存池。

## 测试
-- 一切正常。
