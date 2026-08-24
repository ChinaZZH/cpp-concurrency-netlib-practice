markdown

# Day15： 网络库day001 封装socket和inetaddr

## 核心收获
-- 1.  这一天是先写出了原生的socket模型，然后再这个基础上将socekt相关封装到clientSocket和ListenSocket上，将socket_addr相关封装到inetAddr上。

-- 2. clientSocket和ListenSocket进行RAII设计实现。构造的时候申请socket文件句柄资源，析构的时候释放，以防止忘记释放。

-- 3. 对clientSocket和ListenSocket进行独占式设计，禁止拷贝构造和赋值运算符重载以防止多个socket共用一个句柄造成程序错误。同时支持移动语义。

-- 4. 尽量维持对象设计的单一职责，使其功能明确。同时尽量让函数只返回一个返回值，也使其功能明确。

-- 5. 之前设计错误，将读换从去char buffer[4096] 放到clientSocket中去。这样造成每一个连接上来的客户端都分配一个4K的内存，等连接多了内存会急剧膨胀。修改为在外部使用 char buffer[4096].

## 网络模型分类

-- 1.  网络模型（事件处理层）一共分几种？

-- 2.  在应用层，高性能网络模型主要分为 Reactor 模型 和 Proactor 模型。Reactor 根据线程池的分配方式，主要有以下 3 种变体：

|模型名称	 | 结构特点	|适用场景 |
|-----------|------------|----------|
|单 Reactor 单线程|	所有的监听、I/O 读写、业务逻辑都在一个线程中完成。|	Redis、Memcached（业务逻辑极快，无阻塞）。|
|单 Reactor 多线程|	一个 Reactor 线程负责监听和 I/O 读写，收到数据后将业务逻辑（计算/DB）交给线程池处理，处理完再由 Reactor 线程发送。	| 业务逻辑较重，但 QPS（每秒查询率）要求不是极高的场景。|
|主从 Reactor 多线程|	主 Reactor 只负责 Accept，将连接分发给多个 SubReactor 线程负责 I/O 读写，业务逻辑再交由独立线程池处理。	|Netty、Nginx 默认架构，适用于高并发、高吞吐的通用后端服务。|
|Proactor 模型|	异步 I/O（如 Windows IOCP、Linux io_uring）。操作系统内核完成 I/O 操作（数据读写）后，主动通知用户线程（回调），用户线程无需主动调用 read/write 等待。|	高并发、大文件传输、对 CPU 利用率要求极高的场景。|

## 代码
-- clientSocket.cpp
-- clientSocket.h

-- ListenSocket.cpp
-- ListenSocket.h

-- InetAddress.cpp
-- InetAddress.h

-- echo_server.cpp

## 测试
-- 一切正常。
