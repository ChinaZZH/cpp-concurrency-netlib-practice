markdown

# Day62： 网络库扩展完善 httpServer支持post同时进行压测在同等情况下和rpcServer进行性能比对

## 核心收获

-- 1. 完善httpServer,使它支持post相关的调用

-- 2. 进行压力测试和rpcServer进行对比，http客户端压测工具使用readme.md目录下的bench_test_http_server.cpp。里面的各种计算方式和rpcClient的压测工具是一致的，排除其他因素影响个具有对比性。



## 具体测试性能数据

-- 1. 客户端50线程，每个线程10000个任务对比， rpcServer 除了p99,p999的延时更高之外，其他数据都优于httpServer.

```cpp
rpcServer 客户端50线程，每个线程10000个任务
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

httpServer 客户端50线程，每个线程10000个任务
start 50 thread and req_per_threads 10000:
all_us vector size(): 500000
Total request: 500000
Total time(second): 6.90958
QPS: 72363.3
Average latency: 671.154
P50 latency: 619
P90 latency: 1160
P99 latency: 1753
P999 latency: 2307

```

-- 2. 客户端20线程，每个线程10000个任务对比， rpcServer 除了p99,p999的延时更高之外，其他数据都优于httpServer.但是相比50线程20线程的数据优势降低，但是劣势也降低了。

```cpp
rpcServer 客户端20线程，每个线程10000个任务
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

httpServer 客户端20线程，每个线程10000个任务
tart 20 thread and req_per_threads 10000:
all_us vector size(): 200000
Total request: 200000
Total time(second): 3.66406
QPS: 54584.3
Average latency: 350.045
P50 latency: 311
P90 latency: 605
P99 latency: 941
P999 latency: 1311

```

-- 3. 客户端10线程，每个线程10000个任务对比，rpcServer各项数据都比httpServer更差了，但是差距不会太大。

```cpp
rpcServer 客户端10线程，每个线程10000个任务
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

httpServer 客户端10线程，每个线程10000个任务
start 10 thread and req_per_threads 10000:
all_us vector size(): 100000
Total request: 100000
Total time(second): 2.15264
QPS: 46454.7
Average latency: 207.82
P50 latency: 187
P90 latency: 323
P99 latency: 523
P999 latency: 930

```

-- 4. 客户端5线程，每个线程10000个任务对比，rpcServer各项数据都比httpServer更差了，但是差距不会太大。

```cpp
rpcServer 客户端5线程，每个线程10000个任务
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

httpServer 客户端5线程，每个线程10000个任务
start 5 thread and req_per_threads 10000:
all_us vector size(): 50000
Total request: 50000
Total time(second): 1.61726
QPS: 30916.6
Average latency: 160.63
P50 latency: 157
P90 latency: 207
P99 latency: 261
P999 latency: 435


```

-- 5. 客户端1线程，每个线程10000个任务对比，rpcServer各项数据都比httpServer更差了，但是差距不会太大。

```cpp
rpcServer 客户端1线程，每个线程10000个任务
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

httpServer 客户端1线程，每个线程10000个任务
start 1 thread and req_per_threads 10000:
all_us vector size(): 10000
Total request: 10000
Total time(second): 0.929272
QPS: 10761.1
Average latency: 92.312
P50 latency: 84
P90 latency: 114
P99 latency: 185
P999 latency: 565


```

## 对比说明

-- 1. RPC 服务在高并发场景下性能明显优于 HTTP 服务，而在低并发时 HTTP 略占优势。详细分析如下：

一、QPS 对比

|线程数	|RPC QPS|	HTTP QPS|	RPC 领先幅度|
|-------|-------|-----------|--------------|
|1	|8,813	|10,761	|-18% (HTTP 更高)|
|5	|23,646	|30,917	|-23% (HTTP 更高)|
|10	|38,333	|46,455	|-17% (HTTP 更高)|
|20	|63,560	|54,584	|+16% (RPC 反超)|
|50	|102,869	|72,363	|+42% (RPC 显著领先)|

-- 低并发（1-10 线程）：HTTP 的 QPS 比 RPC 高 17%~23%。原因可能是 RPC 的二进制编解码（长度前缀+字段解析）在轻负载时开销占比相对较高，

	而 HTTP 的文本解析虽然慢但实现简单（如按行分割、状态机），且连接数少时上下文切换少。


-- 高并发（20-50 线程）：RPC 反超并大幅领先。这是因为 RPC 使用二进制协议和长度前缀拆包，减少了数据量和解析复杂度；

	同时 RPC 客户端每个线程独立连接，避免了 HTTP 的头部冗余和复杂解析（如 Content-Length、chunked）。

	50 线程时，RPC QPS 达到 10.3 万，比 HTTP 的 7.2 万高出 42%。

二、延迟对比（以 50 线程为例）

|指标|	RPC	|HTTP|	对比|
|-------|-------|-----------|--------------|
|平均延迟	|414.7 µs|	671.2 µs|	RPC 低 38%|
|P50 延迟|	304 µs|	619 µs|	RPC 低 51%|
|P90 延迟|	747 µs|	1,160 µs|	RPC 低 36%|
|P99 延迟|	2,307 µs|	1,753 µs|	HTTP 反而更低？|

注意：50 线程时 RPC 的 P99 延迟（2.3ms）高于 HTTP（1.75ms），这可能是由于 RPC 在极高并发下的资源竞争（如 pending_ 锁、promise/future 唤醒开销）导致尾部延迟略高。但平均和 P50 延迟 RPC 明显占优。

## 结论
-- 1. RPC 的优势：在高并发（多线程、多连接）下，二进制协议+长度前缀拆包带来显著的吞吐量和平均延迟优势。50 线程时 QPS 高达 10.2 万，比 HTTP 高 42%，平均延迟低 38%。

-- 2. HTTP 的优势：在低并发（1-10 线程）时，实现简单（如按行分割）且无额外编解码开销，QPS 略高于 RPC。

-- 3. 适用场景：如果您的服务需要支撑高并发（如游戏服务器、微服务网关），RPC 是明显更优的选择；如果并发较低且希望通用性好，HTTP 也可以接受。

## 测试
-- 一切正常。
