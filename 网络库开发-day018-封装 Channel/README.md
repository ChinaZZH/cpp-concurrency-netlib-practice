markdown

# Day16： 网络库day002 封装Epoll

## 核心收获
-- 1.  这一天是先写出了原生的socket模型，然后再这个基础上将socekt相关封装到clientSocket和ListenSocket上，将socket_addr相关封装到inetAddr上。

-- 2. clientSocket和ListenSocket进行RAII设计实现。构造的时候申请socket文件句柄资源，析构的时候释放，以防止忘记释放。

-- 3. 对clientSocket和ListenSocket进行独占式设计，禁止拷贝构造和赋值运算符重载以防止多个socket共用一个句柄造成程序错误。同时支持移动语义。

-- 4. 尽量维持对象设计的单一职责，使其功能明确。同时尽量让函数只返回一个返回值，也使其功能明确。

-- 5. 之前设计错误，将读换从去char buffer[4096] 放到clientSocket中去。这样造成每一个连接上来的客户端都分配一个4K的内存，等连接多了内存会急剧膨胀。修改为在外部使用 char buffer[4096].

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
