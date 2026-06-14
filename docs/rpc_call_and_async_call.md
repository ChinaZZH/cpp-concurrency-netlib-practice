markdown

#  rpc服务器同步和异步性能对比

## 分析
|线程数	|同步 QPS	|异步 QPS|	QPS 提升倍数|	同步平均延迟 (us)	|异步平均延迟 (us)|延迟恶化倍数|
|-------|-------------|----------|-----------------|--------------------|------------------|--------------|
|50	145,082	|403,767	|2.78x	|315	|289,521	|919x|
|20	97,453	|321,052	|3.29x	|193	|231,344	|1,199x|
|10	54,957 |	355,722	|6.47x	|178	|88,097	|495x|
|1	12,374	|117,894	|9.53x	|80	|29,505	|369x|

## 结论
-- 1. 异步 RPC 在所有并发下 QPS 均显著高于同步，最高提升 9.5 倍（单线程）。

		随着线程数增加，异步 QPS 提升倍数下降（50 线程时仅 2.78 倍），

		说明高并发下异步模型受限于服务端处理能力或客户端资源竞争。

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
Total request: 500000
Success: 500000 , Timeout: 0 , Fail:0
Total time(second): 1.23834
QPS: 403767
Average latency: 289521
P50 latency: 252003
P90 latency: 573675
P99 latency: 834202
P999 latency: 883843

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
Total request: 200000
Success: 200000 , Timeout: 0 , Fail:0
Total time(second): 0.622953
QPS: 321052
Average latency: 231344
P50 latency: 218331
P90 latency: 381096
P99 latency: 467727
P999 latency: 486976

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
start 10 thread and req_per_threads 10000: 
Total request: 100000
Success: 100000 , Timeout: 0 , Fail:0
Total time(second): 0.281118
QPS: 355722
Average latency: 88097.4
P50 latency: 72289
P90 latency: 164440
P99 latency: 191039
P999 latency: 195012

```

-- 4. 客户端1线程，每个线程10000个任务对比， QPS和总时间数都领先，不过延时却是高的很。

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
start 1 thread and req_per_threads 10000: 
Total request: 10000
Success: 10000 , Timeout: 0 , Fail:0
Total time(second): 0.0848216
QPS: 117894
Average latency: 29504.8
P50 latency: 33423
P90 latency: 39339
P99 latency: 41993
P999 latency: 42283


```



## 结论
-- 1. 异步的qps和总的时间长度都优于同步的，但是延时步入同步。

-- 2. 不过任务越少延时差距越小，说明瓶颈在服务端，任务越多在服务端排队的任务就越多，时长也越长。


## 测试
-- 一切正常。


