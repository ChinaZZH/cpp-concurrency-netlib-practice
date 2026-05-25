markdown

# Day47： 网络库扩展完善 多线程EventLoop设计

## 核心收获
-- 1.  EventLoopThread 职责

--  创建一个独立线程，并在该线程中运行 EventLoop::loop()。

--  提供 startLoop() 方法：启动线程，等待 EventLoop 对象创建完成，然后返回其指针。

--  析构时安全退出：唤醒 EventLoop 的 quit() 并等待线程结束。

关键点：

使用条件变量同步主线程与子线程，确保 startLoop() 返回前 loop_ 已初始化。

子线程栈上创建 EventLoop，生命周期与线程绑定。

支持初始化回调（如设置线程名、执行额外配置）。

-- 2. EventLoopThreadPool 职责

-- 管理一组 EventLoopThread，实现“one loop per thread”模型。

-- setThreadNum() 设置工作线程数量（通常为 CPU 核心数或稍多）。

-- start() 创建指定数量的 EventLoopThread 并启动，收集每个线程的 EventLoop 指针。

-- getNextLoop() 采用简单轮询（round-robin）分配 EventLoop，用于将新连接分发到不同的工作线程。

-- 主线程的 baseLoop_ 不参与工作线程池，通常用于 accept 新连接。


-- 3. 与现有 TcpServer 的集成思路。

-- TcpServer 持有 EventLoopThreadPool 对象。

-- 在 handleNewConnection 中，调用 threadPool_->getNextLoop() 获取一个工作线程的 EventLoop。

-- 使用 ioLoop->runInLoop([conn] { conn->ConnectEstablished(); }) 将 TcpConnection 的初始化投递到正确的 IO 线程，避免跨线程访问 Channel。


## 代码


## 测试
-- 一切正常。
