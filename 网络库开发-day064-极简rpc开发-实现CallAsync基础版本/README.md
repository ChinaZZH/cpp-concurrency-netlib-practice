markdown

# Day62： 网络库扩展完善 实现异步调用的基础 CallAsync

## 核心收获

-- 1. 实现异步CallAsync的rpc请求的调用

-- 2. 进行压力测试和同步的进行数据对比。



## 具体测试性能数据

-- 1. 客户端50线程，每个线程10000个任务对比. QPS和总时间数都领先，不过延时却是高的很。

```cpp
同步
start 50 thread and req_per_threads 10000: 
all_us vector size(): 500000
Total request: 500000
Total time(second): 3.44632
QPS: 145082
Average latency: 314.535
P50 latency: 221
P90 latency: 604
P99 latency: 1812
P999 latency: 4200

异步
start 50 thread and req_per_threads 10000: 
all_us vector size(): 500000
Total request: 500000
Total time(second): 1.08278
QPS: 461774
Average latency: 304516
P50 latency: 299713
P90 latency: 537881
P99 latency: 660453
P999 latency: 671985

```

-- 2. 客户端20线程，每个线程10000个任务对比， QPS和总时间数都领先，不过延时却是高的很。

```cpp
同步
start 20 thread and req_per_threads 10000: 
all_us vector size(): 200000
Total request: 200000
Total time(second): 2.05227
QPS: 97452.8
Average latency: 193.281
P50 latency: 169
P90 latency: 313
P99 latency: 589
P999 latency: 1021

异步
start 20 thread and req_per_threads 10000: 
all_us vector size(): 200000
Total request: 200000
Total time(second): 0.451954
QPS: 442523
Average latency: 172499
P50 latency: 169672
P90 latency: 287643
P99 latency: 360613
P999 latency: 367596

```

-- 3. 客户端10线程，每个线程10000个任务对比， QPS和总时间数都领先，不过延时却是高的很。

```cpp
同步
start 10 thread and req_per_threads 10000: 
all_us vector size(): 100000
Total request: 100000
Total time(second): 1.81959
QPS: 54957.3
Average latency: 177.861
P50 latency: 164
P90 latency: 249
P99 latency: 433
P999 latency: 954

异步
tart 10 thread and req_per_threads 10000: 
all_us vector size(): 100000
Total request: 100000
Total time(second): 0.226425
QPS: 441647
Average latency: 78096.7
P50 latency: 68853
P90 latency: 137484
P99 latency: 166149
P999 latency: 167745

```

-- 4. 客户端5线程，每个线程10000个任务对比， QPS和总时间数都领先，不过延时却是高的很。

```cpp
同步
start 1 thread and req_per_threads 10000: 
all_us vector size(): 10000
Total request: 10000
Total time(second): 0.80813
QPS: 12374.3
Average latency: 80.0587
P50 latency: 74
P90 latency: 95
P99 latency: 170
P999 latency: 294

异步
start 5 thread and req_per_threads 10000: 
all_us vector size(): 50000
Total request: 50000
Total time(second): 0.114521
QPS: 436603
Average latency: 34179.2
P50 latency: 33306
P90 latency: 57304
P99 latency: 63938
P999 latency: 65163


```



## 结论
-- 1. 异步的qps和总的时间长度都优于同步的，但是延时步入同步。

-- 2. 不过任务越少延时差距越小，说明瓶颈在服务端，任务越多在服务端排队的任务就越多，时长也越长。


## 测试
-- 一切正常。
