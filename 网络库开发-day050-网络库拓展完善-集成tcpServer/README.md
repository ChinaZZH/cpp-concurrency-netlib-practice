markdown

# Day40： 网络库扩展完善 将eventloopthread 和 eventloopthreadpool集成到tcpServer中去

## 核心收获

-- 1. 与现有 TcpServer 的集成思路。

-- TcpServer 持有 EventLoopThreadPool 对象。

-- 在 handleNewConnection 中，调用 threadPool_->getNextLoop() 获取一个工作线程的 EventLoop。

-- 使用 ioLoop->runInLoop([conn] { conn->ConnectEstablished(); }) 将 TcpConnection 的初始化投递到正确的 IO 线程，避免跨线程访问 Channel。


## 代码


## 测试
-- 一切正常。
