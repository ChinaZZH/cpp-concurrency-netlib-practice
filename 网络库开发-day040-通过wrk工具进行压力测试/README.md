markdown

# Day40： 网络库day014 用wrk进行压测

## 核心收获
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



# 第一轮调优
-- 1. 修改内核参数（需要 sudo）

    增大全连接队列长度（同时影响 listen 的 backlog）
    sudo sysctl -w net.core.somaxconn=32768，同时调整 ListenSocket::listen 的 backlog 参数。
    
    ListenSocket::Listen(int backlog) 中，将 backlog 设置为与 somaxconn 相同的值（如 32768），两者取最小值才是实际队列长度。


    增大半连接队列长度（应对 SYN 洪水）
    sudo sysctl -w net.ipv4.tcp_max_syn_backlog=8192

    开启 TIME_WAIT 快速重用（需配合 tcp_timestamps）
    sudo sysctl -w net.ipv4.tcp_tw_reuse=1
    sudo sysctl -w net.ipv4.tcp_timestamps=1

    扩大本地端口范围（减少端口耗尽风险）
    sudo sysctl -w net.ipv4.ip_local_port_range="1024 65535"

    设置端口数目 ulimit -n 65535，确保在服务器启动前设置（可以在启动脚本中加入）


-- 2. 同时使用高低水位。这一来可以控制outputBuffer的最长限度在64KB附近，同时也能够平缓outputBuffer写入内核缓冲区的速率，起到削峰填谷的作用。

-- 3. 下面是一轮优化后的状态数据:
 wrk -t4 -c1000 -d100s http://localhost:8888

Running 2m test @ http://localhost:8888
  
  4 threads and 1000 connections
  
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    
    Latency    30.40ms   30.53ms   1.98s    99.86%
    
    Req/Sec     8.11k   760.83    11.37k    83.60%
  
  3228462 requests in 1.67m, 274.02MB read
  
  Socket errors: connect 0, read 0, write 0, timeout 565

Requests/sec:  32257.39

Transfer/sec:      2.74MB



# 第二轮优化
-- 1. 减少 HttpContext 中的内存分配与拷贝, 预分配 HttpContext 对象，每个连接复用一次

-- 2. 在HttpContext在解析过程中 使用 std::string_view 代替 std::string=substr() 减少内存复制和拷贝。

-- 3. HttpContext 里面的header解析后的数据用 std::vector 替代 std::unordered_map。 因为header数据较少，直接遍历用了计算机CPU更高级的缓存会比哈希更快。

-- 4. 下面是一轮优化后的状态数据:

wrk -t4 -c1000 -d100s http://localhost:8888

Running 2m test @ http://localhost:8888
  
  4 threads and 1000 connections
  
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    
    Latency    27.49ms   31.92ms   2.00s    99.84%
    
    Req/Sec     9.07k   727.60    12.38k    84.30%
  
  3611571 requests in 1.67m, 306.54MB read
  
  Socket errors: connect 0, read 0, write 0, timeout 556

Requests/sec:  36081.50

Transfer/sec:      3.06MB


# 第三轮优化（数据反馈未见提升）

-- 1. 系统级调优补充

    增大 net.ipv4.tcp_rmem 和 net.ipv4.tcp_wmem 的默认和最大缓冲区，例如：
    sudo sysctl -w net.ipv4.tcp_rmem="4096 87380 16777216"
    sudo sysctl -w net.ipv4.tcp_wmem="4096 65536 16777216"

    开启 net.core.netdev_max_backlog 提高网卡队列长度：
    sudo sysctl -w net.core.netdev_max_backlog=5000
    使用 taskset 将服务进程绑定到固定 CPU 核心，减少上下文切换

-- 2.下面是一轮优化后的状态数据:
 wrk -t4 -c1000 -d100s http://localhost:8888

Running 2m test @ http://localhost:8888
  
  4 threads and 1000 connections
  
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    
    Latency    28.09ms   30.10ms   1.99s    99.86%
    
    Req/Sec     8.78k   754.98    16.84k    83.87%
  
  3493231 requests in 1.67m, 296.50MB read
  
  Socket errors: connect 0, read 0, write 0, timeout 593

Requests/sec:  34896.49

Transfer/sec:      2.96MB


# 第四轮优化
-- 1.可以适当增大输出缓冲区 OutBuffer 的初始大小（例如 8192)。



# 同步发送方式  当前优化效果总结
|指标|	    优化前|	第一次优化后|	变化 |        第二次优化后    |  变化   |       第三次优化后|    变化        |   第四次优化后 |       变化|
|----|------------|------------|--------|------------------------|--------|--------------------|----------------|----------------|------------|
|QPS	 |       13656|     32,257|	  +136.2%   |    36,082	      |  +11.8%   |      36081     |     基本持平    |   35944     |        基本持平|
|平均延迟| 	38.73ms|    30.40ms|	   -21.6% |       27.49ms	|     -9.6%    |      28.09ms  |       基本持平 |     27.60ms  |         基本持平 |
|超时	 |   644   |       565	 | -12.3%     |       556	    |        基本持平  |      556    |     基本持平  |     495     |         -11%|
|延时标准差|  62.25ms  |   30.53 |     -51%   |        31.92ms  |         基本持平  |      30.10ms |    基本持平|        29.12  |          基本持平|


# 在上面的基础上将httpServer发送给客户端的方式优化成异步
-- 1.线程池的线程个数为核心数-1

 wrk -t4 -c1000 -d100s http://localhost:8888

Running 2m test @ http://localhost:8888
  
  4 threads and 1000 connections
  
  Thread Stats   Avg      Stdev     Max   +/- Stdev
    
    Latency    22.64ms   36.66ms   2.00s    99.79%
    
    Req/Sec    11.44k     1.34k   41.41k    83.49%
  
  4550982 requests in 1.67m, 386.27MB read
  
  Socket errors: connect 0, read 0, write 0, timeout 505

Requests/sec:  45465.59

Transfer/sec:      3.86MB

# 将httpServer传递给线程池的由fd 修改为weak_ptr<TcpConnection>
 
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







