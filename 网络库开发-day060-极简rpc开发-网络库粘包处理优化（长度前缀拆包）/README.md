markdown

# Day60： 网络库扩展完善 网络库粘包处理优化（长度前缀拆包）

## 核心收获

-- 1. 实现rpcServer网络库粘包处理，使用长度前缀拆包

-- 2. 修复rpcClient的rpc_timeout超时问题。

-- 3. rpcClient超时问题. 在消息发送前，就需要将promise存入，而不是发送后。因为消息回包可能在存入promise之前。(发送后CPU切换直到很晚才切到存入promise逻辑)

-- 4. 使用火焰图 perf 分析CPU性能的瓶颈，发现性能瓶颈在json的拆包和解包，后期会使用protobuf来进行相对应的优化。

-- 5. 同级目录有对应的火焰图。


## 压测性能
 
start 50 thread and req_per_threads 10000: 

all_us vector size(): 500000

Total request: 500000

Total time(second): 5.94875

QPS: 84051.3

Average latency: 570.12

P50 latency: 390

P90 latency: 1088

P99 latency: 3470

P999 latency: 7279



start 20 thread and req_per_threads 10000: 

all_us vector size(): 200000

Total request: 200000

Total time(second): 3.21967

QPS: 62118.1

Average latency: 307.649

P50 latency: 268

P90 latency: 488

P99 latency: 886

P999 latency: 1599



start 10 thread and req_per_threads 10000: 

all_us vector size(): 100000

Total request: 100000

Total time(second): 2.52019

QPS: 39679.6

Average latency: 247

P50 latency: 232

P90 latency: 338

P99 latency: 515

P999 latency: 923



start 5 thread and req_per_threads 10000: 

all_us vector size(): 50000

Total request: 50000

Total time(second): 2.19689

QPS: 22759.5

Average latency: 217.344

P50 latency: 208

P90 latency: 276

P99 latency: 430

P999 latency: 871



start 1 thread and req_per_threads 10000: 

all_us vector size(): 10000

Total request: 10000

Total time(second): 1.04749

QPS: 9546.63

Average latency: 104.027

P50 latency: 90

P90 latency: 144

P99 latency: 238

P999 latency: 538


## 代码


## 测试
-- 一切正常。
