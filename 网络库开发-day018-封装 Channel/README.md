markdown

# Day18： 网络库day003 封装Channel

## 核心收获
-- 1.  修改与Chanel相关的Epoll, AddFd 和 ModifyFd 传参由优先的 int fd 修改为 Channel* ptr, 同时Wait返回的也是一个Channel* 列表。

-- 2.  Channel的作用是对epoll_event的事件进行分发和调度，通过注册事件回调函数同时识别epoll上发生的是什么事件来进行调度。

-- 3. Channel的意思就是通道，就是连接ListenSocket/ClientSocket的相关桥梁。但是跟 ListenSocket/ClientSocket不同的是， 

      Channel 是关联句柄id与其对应的调度事件(读写事件，异常事件)。而ListenSocket/ClientSocket关联的是句柄id和socket操作。具体可以看代码回顾。

-- 4. 使用std::unique_ptr 来代替纯指针，来让只能指针帮助管理内存，用来实现RAII机制。

-- 5. g_epoll 执行 del 操作需要在close(fd) 之前不然会报错。 

## 代码
-- Channel.cpp
-- Channel.h

## 测试
-- 一切正常。
