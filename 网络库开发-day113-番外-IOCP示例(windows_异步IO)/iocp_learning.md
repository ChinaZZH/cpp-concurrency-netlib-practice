markdown
# IOCP 学习笔记

## 概述

IOCP（Input/Output Completion Port，完成端口）是 Windows 平台下最高效的 I/O 模型，用于处理高并发网络通信。它属于 **Proactor（异步 I/O）** 模型，与 Linux 下的 **epoll（Reactor）** 模型形成对比。

**核心特点：**
- 少量工作线程处理大量并发连接
- 异步 I/O 操作由内核完成，完成后通知用户线程
- 避免“一个连接一个线程”的资源浪费
- 适合高并发、高性能的服务器开发

---

## 核心概念

### 1. 完成端口（Completion Port）

完成端口是一个由内核管理的 **I/O 完成通知队列**。

- 当异步 I/O 操作完成时，系统将完成事件放入队列
- 工作线程从队列中取出事件并处理
- 一个完成端口可以关联多个套接字句柄

> 可以理解为：系统负责“把完成通知放进信箱”，工作线程负责“从信箱取信处理”。

### 2. 重叠 I/O（Overlapped I/O）

重叠 I/O 是 Windows 异步 I/O 的基础机制。

- 发起 I/O 操作后函数立即返回，不等待操作完成
- 每个异步 I/O 操作需要一个 `OVERLAPPED` 结构体
- 操作完成后通过完成端口通知应用程序

### 3. 工作线程池

工作线程是实际处理 I/O 完成通知的线程。

- 线程数量通常设为 CPU 核心数的 2 倍
- 每个线程循环调用 `GetQueuedCompletionStatus` 等待事件
- 收到通知后取出数据并处理

---

## 核心 API

| API | 作用 | 调用时机 |
| :--- | :--- | :--- |
| `CreateIoCompletionPort` | ① 创建完成端口 ② 将句柄关联到完成端口 | 程序启动时创建；新连接建立时关联 |
| `GetQueuedCompletionStatus` | 从完成端口取出一个完成通知（阻塞等待） | 工作线程主循环 |
| `WSARecv` / `WSASend` | 发起异步 I/O 请求（立即返回） | 需要收发数据时 |
| `AcceptEx` | 异步接受客户端连接（需要动态加载） | 投递异步 Accept 请求 |
| `PostQueuedCompletionStatus` | 向完成端口发送自定义完成事件 | 退出时唤醒工作线程 |


## 关键数据结构

### PER_IO_CONTEXT（每个 I/O 操作的上下文）

```cpp
struct PER_IO_CONTEXT {
    OVERLAPPED overlapped;      // 必须放在第一个字段
    WSABUF wsa_buf;             // 数据缓冲区
    char buffer[BUFFER_SIZE];   // 实际存储数据的缓冲区
    SOCKET client_socket;       // AcceptEx 专用：存储新连接的 Socket
    PER_SOCKET_CONTEXT* owner;  // 指向所属的套接字上下文
};
```
PER_SOCKET_CONTEXT（每个套接字的上下文）
```cpp
cpp
struct PER_SOCKET_CONTEXT {
    SOCKET socket;
    PER_IO_CONTEXT recv_io;     // 接收上下文
    PER_IO_CONTEXT send_io;     // 发送上下文
    bool is_closed;
};
```

设计要点：recv_io 和 send_io 必须独立，因为接收和发送是并行进行的异步操作，不能共用一个 OVERLAPPED。

## 核心流程
### 服务端启动流程
```text
WSAStartup() → 创建完成端口 → 创建监听 Socket → 加载 AcceptEx → 绑定端口 → 监听
→ 将监听 Socket 关联到 IOCP → 投递第一个 AcceptEx → 启动工作线程池
```

### 连接处理流程
```text
AcceptEx 完成 → 创建 PER_SOCKET_CONTEXT → 客户端 Socket 关联到 IOCP
→ 投递第一个 WSARecv → 投递下一个 AcceptEx（循环）
```

### Echo 收发流程
```text
WSARecv 完成 → 打印数据 → 投递下一个 WSARecv（提前投递，让接收持续进行）
→ WSASend 发送数据（Echo）→ 发送完成后记录日志
```

## 与 epoll 的对比
|维度	| epoll（Linux）|	IOCP（Windows）|
|-------|-----------------|---------------------|
|I/O 模型	| Reactor（同步非阻塞）|	Proactor（异步）|
|通知内容	|“可以读/写了”（就绪通知）	|“已经读/写完了”（完成通知）|
|数据拷贝	| 应用层收到通知后调用 recv/send 主动拷贝 |	内核完成拷贝，用户态直接使用数据|
|工作线程职责	| 监听事件 → 读写 → 处理业务 |	发起请求 → 等待通知 → 使用数据|
|适用平台|	Linux	| Windows |

## 一句话总结：

epoll 通知你“可以动手了”，IOCP 通知你“已经搞定了”。





