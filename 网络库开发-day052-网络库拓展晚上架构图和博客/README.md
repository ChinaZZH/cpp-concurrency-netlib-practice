markdown

# Day52： 网络库扩展完善 (在day42的博客基础上)

## 1. 背景与目标

-- 背景: 通过多线程来分担单个线程读写的压力，更好的承受高连接和大流量。 

-- 目标: 实现高并发 HTTP 服务器，支持同步/异步处理，压测达到 7.7w QPS。

## 2. 整体架构

<img width="760" height="710" alt="网络库拓展" src="https://github.com/user-attachments/assets/4002ba32-5e47-4168-b129-0ab37cd07139" />




## 3. 数据流简述（直接写在图下方）：

新连接：
Main EventLoop 监听到 EPOLLIN → accept → 创建 TcpConnection → 从 EventLoopThreadPool 获取一个工作 EventLoop → 通过 runInLoop 将 connectEstablished 投递到工作线程 → 工作线程将 Channel 注册到自己的 Epoll，并将 TcpConnection 存入自己的连接集合。

数据到达：
工作线程的 EventLoop 监听到 EPOLLIN → 对应 Channel 的读回调 → TcpConnection::HandleRead → inputBuffer_.readFd() → 调用 MessageCallback（可能投递给业务线程池处理）。

响应发送：
业务线程处理完后，通过 conn->getLoop()->runInLoop 将 Send 任务投递回工作线程 → outputBuffer_.append → 注册 EPOLLOUT → 工作线程可写事件触发 → HandleWrite → 发送数据。

连接关闭：
TcpConnection::handleClose 在自己的工作线程中执行：从 EventLoop 的连接集合中删除 shared_ptr（最后析构），同时调用 CloseCallback 通知主线程清理 fd -> EventLoop* 轻量映射（若需要）。

## 4. 核心模块：

主线程（Main Reactor）

包含：EventLoop（baseLoop_）、ListenSocket、Channel（监听 fd）

职责：accept 新连接，然后将连接分配给工作线程


工作线程池（Sub Reactors）

由 EventLoopThreadPool 管理，每个线程一个 EventLoop

每个 EventLoop 拥有：Epoll、Channel 管理、pendingFunctors_ 队列、自己的连接集合（std::map<int, std::shared_ptr<TcpConnection>>）


连接对象（TcpConnection）

包含：ClientSocket、Channel、Buffer（输入/输出）、lastActiveTime_

回调：MessageCallback、CloseCallback、WriteCompleteCallback


业务线程池

独立的 ThreadPool，用于执行耗时业务逻辑（如 HttpServer::onMessage）


## 5. 关键实现细节
### 5.1 定时器
- 通过timer系统文件句柄实现内核驱动调度定时器
- 通过 std::multimap 管理根据系统时间由小到大的定时器时间点和定时器事件。
- 通过递归调用的方式实现周期性定时器。

### 5.2 空闲超时
-  当创建TcpConnection的时候以及有读写事件的时候更新活跃时间
-  周期性每隔5秒遍历所有TcpConnecton，判断 非活跃时间是否已经达到限定的阈值(60秒)。若是就关闭该连接。

### 5.3 优雅关闭
- 当tcpConnection空闲超时需要服务端主动关闭的时候，采用优雅关闭。服务端主动关闭写操作(从全连接变半连接)同时保持读操作的通道。等待客户端发起关闭全连接的操作。

### 5.4 多线程EventLoop
- 通过多个EventLoop多线程 来分担单个eventLoop的读写压力。同时能更好的利用CPU多核的优势。

- 目前connection_list由tcp管理。当工作线程需要insert或者erase连接容器的时候需要通过 主线程eventLoop的runInLoop进行投递。

-- 同时当主线程需要对channel进行相关操作的时候也需要工作线程的EventLoop进行相关的runInLoop进行投递。

## 6. 优化与压测
- 压测结果（`wrk -t4 -c1000 -d30s`）：

|工作线程数|	QPS|	平均延迟|	P99 延迟|	超时数|	CPU 总占用|	CPU 核心负载分布|
|----------|-----|----------|---------|-------|-----------|------------------|
|0（单线程）|	40,362	|25.98ms	|44.46ms	|399|	~140%	|极不均衡（单核高负载）|
|2|	60,559	|16.61ms	|36.33ms	|0	|~500%	|较均衡|
|4|	71,521	|14.10ms	|32.99ms	|0	|~429%	|很均衡|
|6|	77,789	|12.92ms	|30.98ms	|0	|~439%	|非常均衡|
|8|	76,275	|13.26ms	|34.17ms	|0	|~433%	|非常均衡|
注：CPU 总占用为 top 中所有 echo_server 线程的 %CPU 总和，反映进程占用的总核心算力。

## 7. 遇到的问题与解决
- **超时自动关闭**：超时自动关闭功能，在tcpConnection构造成功和onEstablished的时候也需要激活当前事件为活跃时间。

- **优雅关闭**：测试优雅关闭需要用C++客户端测试，python客户端测试不准确。

- **Channel::HandleEvent单次多个事件**：单Channel::HandleEvent有多个事件一起发上来的时候，需要按照顺序执行，先读写，然后是关闭，最后才是异常处理。

- **多线程eventLoop**：需要明确不同的操作需要在哪个eventLoop线程执行，若需要在其他线程执行需要通过其他线程的eventLoop->runInLoop进行投递执行函数。
 
## 6. 总结与展望
- 收获：多线程reactor
- 后续：io_uring、RPC 扩展

## 7. 代码仓库
[GitHub 链接](https://github.com/ChinaZZH/cpp-concurrency-netlib-practice)

## 8. 参考
- muduo 网络库
- 《Linux 高性能服务器编程》
 
 
