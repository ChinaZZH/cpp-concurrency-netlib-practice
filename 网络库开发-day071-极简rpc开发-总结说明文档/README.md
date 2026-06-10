markdown

# Day71： 网络库扩展完善 阶段性总结

# C++ 高性能网络库与 RPC 框架

## 项目简介
- 从零实现基于 epoll + 多线程 Reactor 的网络库
- 在此基础上构建完整的 RPC 框架，支持同步/异步调用、超时、连接池、服务发现、链路追踪

## 核心特性
### 网络库
- 多线程 EventLoop（主从 Reactor）
- 非阻塞 I/O + 边缘触发（ET）
- 定时器（`RunAfter`/`RunEvery`）
- 缓冲区（`Buffer`）支持 `readv`/`writev`
- 连接水位回调（高/低水位控制）

### RPC 框架
- 同步调用（`Call`）+ 超时 + 错误码
- 异步调用（`CallAsync`）+ 回调 + 超时清理
- 连接池（轮询负载均衡，自动剔除失效连接）
- 服务发现（基于 HTTP 注册中心，心跳保持）
- 链路追踪（`trace_id` 全链路传递）
- 编解码：Protobuf + 长度前缀（粘包处理）



## 性能测试
| 场景 | QPS | P99 延迟 | 环境 |
|:---|:---:|:---:|:---|
| RPC 同步调用 | 16 万 | 1.4 ms | 50 线程 |
| RPC 异步调用 | 44 万 | 300 μs | 50 线程 |


## 快速开始
### 环境要求
- C++17
- Linux（epoll）
- Protobuf
- nlohmann/json

### 编译运行
```bash
mkdir build && cd build
cmake .. && make
./echo_server  # 启动服务端
./rpc_client   # 启动客户端