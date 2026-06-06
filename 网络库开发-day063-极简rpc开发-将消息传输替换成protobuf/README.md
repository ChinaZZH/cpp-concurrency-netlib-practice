markdown

# Day63： 网络库扩展完善 替换protobuf

## 核心收获

-- 1. 消息传递由json替换成protobuf

-- 2. 进行压力测试和替换前的对比



## 具体测试性能数据

-- 1. 客户端50线程，每个线程10000个任务对比， 替换前json匹配 除了p99,p999的延时更高之外，其他数据都优于替换后proto匹配.

```cpp
替换前json匹配 客户端50线程，每个线程10000个任务
start 50 thread and req_per_threads 10000:
all_us vector size(): 500000
Total request: 500000
Total time(second): 4.86057
QPS: 102869
Average latency: 414.717
P50 latency: 304
P90 latency: 747
P99 latency: 2307
P999 latency: 6114

替换后proto匹配 客户端50线程，每个线程10000个任务
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
```

-- 2. 客户端20线程，每个线程10000个任务对比， 替换前json匹配 除了p99,p999的延时更高之外，其他数据都优于替换后proto匹配.但是相比50线程20线程的数据优势降低，但是劣势也降低了。

```cpp
替换前json匹配 客户端20线程，每个线程10000个任务
start 20 thread and req_per_threads 10000:
all_us vector size(): 200000
Total request: 200000
Total time(second): 3.14661
QPS: 63560.4
Average latency: 298.364
P50 latency: 259
P90 latency: 495
P99 latency: 989
P999 latency: 1884

替换后proto匹配 客户端20线程，每个线程10000个任务
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

```

-- 3. 客户端10线程，每个线程10000个任务对比，替换前json匹配各项数据都比替换后proto匹配更差了，但是差距不会太大。

```cpp
替换前json匹配 客户端10线程，每个线程10000个任务
start 10 thread and req_per_threads 10000:
all_us vector size(): 100000
Total request: 100000
Total time(second): 2.60875
QPS: 38332.5
Average latency: 255.299
P50 latency: 236
P90 latency: 381
P99 latency: 605
P999 latency: 969

替换后proto匹配 客户端10线程，每个线程10000个任务
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


```

-- 4. 客户端5线程，每个线程10000个任务对比，替换前json匹配各项数据都比替换后proto匹配更差了，但是差距不会太大。

```cpp
替换前json匹配 客户端5线程，每个线程10000个任务
start 5 thread and req_per_threads 10000:
all_us vector size(): 50000
Total request: 50000
Total time(second): 2.11453
QPS: 23645.9
Average latency: 209.712
P50 latency: 206
P90 latency: 263
P99 latency: 328
P999 latency: 628

替换后proto匹配 客户端5线程，每个线程10000个任务
start 5 thread and req_per_threads 10000: 
all_us vector size(): 50000
Total request: 50000
Total time(second): 1.57378
QPS: 31770.7
Average latency: 155.754
P50 latency: 153
P90 latency: 199
P99 latency: 249
P999 latency: 390


```

-- 5. 客户端1线程，每个线程10000个任务对比，替换前json匹配各项数据都比替换后proto匹配更差了，但是差距不会太大。

```cpp
替换前json匹配 客户端1线程，每个线程10000个任务
start 1 thread and req_per_threads 10000:
all_us vector size(): 10000
Total request: 10000
Total time(second): 1.13467
QPS: 8813.17
Average latency: 112.755
P50 latency: 107
P90 latency: 122
P99 latency: 199
P999 latency: 867

替换后proto匹配 客户端1线程，每个线程10000个任务
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


```


## 结论
-- 1. json替换成proto各项数据都有显著提升


## 测试
-- 一切正常。
