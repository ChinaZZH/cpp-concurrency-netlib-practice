markdown

# Day78： 极致性能优化 哈希表的替换

## 核心收获

-- 1. 将 std::unordered_map 和  std::unordered_set 替换成 absl::flat_hash_map 和 absl::flat_hash_set


## 具体测试性能数据
-- 1.  客户端异步调用 AsyncCall. 100线程，每个线程10000个任务对比. 各项数据都下降了

```cpp
使用 std::unordered
start Async Call 100 thread and req_per_threads 10000:
CallAsync  Total request: 1000000
Success: 1000000 , Timeout: 0 , Fail:0
Total time(second): 3.1546
QPS: 316998
Average latency: 1.0578e+06
P50 latency: 1012329
P90 latency: 1786151
P99 latency: 2381283
P999 latency: 2842532

使用 absl::flat_hash
start Async Call 100 thread and req_per_threads 10000:
CallAsync  Total request: 1000000
Success: 1000000 , Timeout: 0 , Fail:0
Total time(second): 4.02985
QPS: 248148
Average latency: 1.76226e+06
P50 latency: 1768277
P90 latency: 2763380
P99 latency: 3436392
P999 latency: 3690990

```

-- 2. 客户端同步调用 100线程，每个线程10000个任务对比， QPS和总时间数都领先，各项数据都下降了。

```cpp
使用 std::unordered
start Call 100 thread and req_per_threads 10000:
Call  Total request: 1000000
Success: 1000000 , Timeout: 0 , Fail:0
Total time(second): 10.2762
QPS: 97312
Average latency: 964.014
P50 latency: 801
P90 latency: 1769
P99 latency: 3447
P999 latency: 6217

使用 absl::flat_hash
start Call 100 thread and req_per_threads 10000:
Call  Total request: 1000000
Success: 1000000 , Timeout: 0 , Fail:0
Total time(second): 11.9085
QPS: 83973.4
Average latency: 1116.7
P50 latency: 917
P90 latency: 2054
P99 latency: 4194
P999 latency: 7606

```

-- 3. 内存消耗
```cpp
使用 std::unordered
Command being timed: "./rpc_server"
        User time (seconds): 94.13
        System time (seconds): 25.07
        Percent of CPU this job got: 251%
        Elapsed (wall clock) time (h:mm:ss or m:ss): 0:47.39
        Average shared text size (kbytes): 0
        Average unshared data size (kbytes): 0
        Average stack size (kbytes): 0
        Average total size (kbytes): 0
        Maximum resident set size (kbytes): 145392
        Average resident set size (kbytes): 0
        Major (requiring I/O) page faults: 0
        Minor (reclaiming a frame) page faults: 36980
        Voluntary context switches: 349759
        Involuntary context switches: 699653
        Swaps: 0
        File system inputs: 0
        File system outputs: 0
        Socket messages sent: 0
        Socket messages received: 0
        Signals delivered: 0
        Page size (bytes): 4096
        Exit status: 0

使用 absl::flat_hash
        Command being timed: "./rpc_server"
        User time (seconds): 106.69
        System time (seconds): 29.71
        Percent of CPU this job got: 435%
        Elapsed (wall clock) time (h:mm:ss or m:ss): 0:31.30
        Average shared text size (kbytes): 0
        Average unshared data size (kbytes): 0
        Average stack size (kbytes): 0
        Average total size (kbytes): 0
        Maximum resident set size (kbytes): 190332
        Average resident set size (kbytes): 0
        Major (requiring I/O) page faults: 0
        Minor (reclaiming a frame) page faults: 49970
        Voluntary context switches: 397235
        Involuntary context switches: 693526
        Swaps: 0
        File system inputs: 0
        File system outputs: 0
        Socket messages sent: 0
        Socket messages received: 0
        Signals delivered: 0
        Page size (bytes): 4096
        Exit status: 0
```

## 结论

--  1. stl标准库中的哈希表已经足够优秀，可以覆盖大部分场景

-- 2.  absl::flat_hash_map 针对高负载、大量查找插入的场景优化，当插入删除次数很少且并发度适中, absl::flat_hash_map带来的优势并没有更好。
		
		反而会因为替换引入的额外依赖（如内存分配策略、哈希算法）可能带来了微小但负面的影响。



## 测试
-- 一切正常。


