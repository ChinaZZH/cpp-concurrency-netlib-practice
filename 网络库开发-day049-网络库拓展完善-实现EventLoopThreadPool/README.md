markdown

# Day49： 网络库扩展完善 多线程EventLoopThreadPool设计

## 核心收获

-- 1. EventLoopThreadPool 职责

-- 管理一组 EventLoopThread，实现“one loop per thread”模型。

-- setThreadNum() 设置工作线程数量（通常为 CPU 核心数或稍多）。

-- start() 创建指定数量的 EventLoopThread 并启动，收集每个线程的 EventLoop 指针。

-- getNextLoop() 采用简单轮询（round-robin）分配 EventLoop，用于将新连接分发到不同的工作线程。

-- 主线程的 baseLoop_ 不参与工作线程池，通常用于 accept 新连接。


## 代码


## 测试
-- 一切正常。
