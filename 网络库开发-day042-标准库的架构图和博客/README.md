markdown

# Day42： 网络库一阶段总结

## 1. 背景与目标

-- 背景: 为了深入理解Reactor模型也和并发编程，也为了能自己动手从零搭建一个网络库来提升自己的信心和能力。 

-- 目标: 实现高并发 HTTP 服务器，支持同步/异步处理，压测达到 4.5w QPS。

## 2. 整体架构

<img width="770" height="590" alt="网络库整体架构图" src="https://github.com/user-attachments/assets/20bcf85d-c9b2-42b9-a7b8-e44753ebdc45" />



数据流简述（直接写在图下方）：

Epoll::wait客户端连接 → ListenSocket → accept → Channel 读事件 → Epoll的Add 同时创建 TcpConnection 注册到 EventLoop 

Epoll::wait数据到达  → Channel 读事件 → TcpConnection::HandleRead → inputBuffer_ → HttpServer::onMessage

同步处理：直接 Send 响应；

异步处理：ThreadPool 执行任务 → runInLoop 回调 → TcpConnection::Send

Send → outputBuffer_ → sendOutput → 注册 EPOLLOUT → 可写事件 → 数据发出

核心模块：
- **EventLoop**：事件循环，负责 epoll 事件分发和跨线程回调
- **TcpConnection**：管理单个连接，包含输入/输出 Buffer
- **TcpServer**：管理所有连接，处理新连接
- **HttpServer**：基于 TcpServer，解析 HTTP 请求，支持同步/异步响应
- **ThreadPool**：异步处理业务逻辑，避免阻塞 IO 线程
- 

## 3. 关键实现细节
### 3.1 事件驱动
- Epoll 边缘触发 + 非阻塞 socket
- Channel 绑定 fd 和回调，EventLoop 统一分发

### 3.2 跨线程唤醒
- `runInLoop` / `queueInLoop` + `eventfd` 唤醒 epoll_wait
- `pendingFunctors_` 队列保证回调在 IO 线程执行

### 3.3 连接生命周期
- `shared_ptr<TcpConnection>` 管理，`runInLoop` 中释放，确保析构在 IO 线程

### 3.4 Buffer 与高低水位
- `std::vector<char>` 动态缓冲区，支持 readv/writev
- 高水位（64KB）暂停发送，低水位（32KB）恢复，削峰填谷

## 4. 优化与压测
- 内核参数：`somaxconn=32768`、`tcp_tw_reuse=1`
- 应用层：HttpContext 复用、string_view、vector 替代 unordered_map
- 压测结果（`wrk -t4 -c1000 -d100s`）：

| 版本 | QPS | 平均延迟 | 超时 |
|------|-----|----------|------|
| 同步 | 36081 | 27.49ms | 556 |
| 异步 | 45639 | 22.51ms | 508 |

## 5. 遇到的问题与解决
- **跨线程析构**：`shared_ptr` 在 `runInLoop` 中释放 → 确保 IO 线程析构
- **唤醒失效**：加入 `eventfd` 并正确读写,要环形的话写入需要写1，写0则没有事件在epoll_wait环形。
- **无锁队列试验**：轻任务下互斥锁+条件变量性能更好。
- **优化性能**：通过修改系统性能参数: 系统设定的全连接和半链接队列的个数 以及高低水位的设置来优化性能。
 
## 6. 总结与展望
- 收获：Reactor 模型、并发控制、性能调优
- 后续：多线程 Reactor、io_uring、RPC 扩展

## 7. 代码仓库
[GitHub 链接](https://github.com/ChinaZZH/cpp-concurrency-netlib-practice)

## 8. 参考
- muduo 网络库
- 《Linux 高性能服务器编程》
 
 
