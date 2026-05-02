markdown

# Day16： 网络库day002 封装Epoll

## 核心收获
-- 1.  在day15的基础上，也就是在echo_server.cpp加入原生的epoll来支持IO多路复用。

-- 2. 将epoll相关的内容提取出来封装到Epoll文件Epoll对象中去。

-- 3. ClientSocket 修改write函数，当一次没有办法写了len字节的时候那就用while循环分多次写入。

-- 4. 支持Epoll中的Add, Del 还有Modify。

-- 5. 查epoll相关函数的参数以及返回值的含义，以保证正确使用。

-- 6. 提高性能的优化。 将客户端socket的 读事件 从水平触发修改成边缘触发， 从原先的读一次修改成读数据知道没有数据为止。

## 代码
-- Epoll.cpp

-- Epoll.h

## 测试
-- 一切正常。
