markdown

# Day42： 网络库一阶段总结

## 网络库架构图
-- 1. 安装wrk，用wrk进行压测 使用 wrk -t4 -c1000 -d100s http://localhost:8888  4个线程 1000个客户端连接持续100秒的测试。

-- 2. 下面是优化前的状态。

wrk -t4 -c1000 -d100s http://localhost:8888

Running 2m test @ http://localhost:8888

  4 threads and 1000 connections
  
|Thread Stats|   Avg |     Stdev |    Max |  +/- Stdev|
|-----------|-----------|-----------|-----------|-----------|
|Latency   | 38.73ms |  62.25ms |  1.97s  |  79.85%|
| Req/Sec    | 4.60k   |  1.25k  | 10.01k  |  71.27%|
    
  1357603 requests in 1.66m, 115.23MB read
  
  Socket errors: connect 0, read 1679, write 11029463, timeout 644
  
Requests/sec:  13656.95

Transfer/sec:      1.16MB



 
 wrk -t4 -c1000 -d100s http://localhost:8888

Running 2m test @ http://localhost:8888
  
  4 threads and 1000 connections
  
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    
    Latency    22.90ms   33.76ms   2.00s    99.75%
    
    Req/Sec    11.28k     1.39k   42.20k    92.52%
  
  4490023 requests in 1.67m, 381.10MB read
  
  Socket errors: connect 0, read 0, write 0, timeout 429

Requests/sec:  44855.56


|指标|	    同步最优情况前|	 优化成异步|	变化 |        异步的第一次优化    |  变化   |       异步的第二次优化|    变化        |   异步的第三次优化 |       变化|
|----|------------|------------|--------|------------------------|--------|--------------------|----------------|----------------|------------|
|QPS	 |       36,082|     45465|	  +26%   |    	44855      |   基本持平 |           |     基本持平    |        |        基本持平|
|平均延迟| 	27.49ms|    22.64ms|	   -17.7% |       22.90	|      基本持平  |       |       基本持平 |       |         基本持平 |
|超时	 |   495   |       505	 | 基本持平     |      429 	    |     -15.1%   |          |     基本持平  |          |         -11%|
|延时标准差|  29.12  |   31.70ms |     基本持平   |    33.76      |    基本持平       |      |    基本持平|          |          基本持平|







