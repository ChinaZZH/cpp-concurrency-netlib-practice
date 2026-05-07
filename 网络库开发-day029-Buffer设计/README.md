markdown

# Day29： 网络库day008 Buffer设计(其他内容后面再修改补充)

## 核心收获
-- 1. TcpConnection不再有Channel的所有权也不再保持Channel的指针。让EventLoop保有Channel的指针和所有权，这样统一管理不会出现已经被删除了野指针还在运行。

-- 2. TcpServer 中的 listen Channel 也一并给 EventLoop管理。

-- 3. 用 std::unique_ptr 来管理Channel的生命周期， erase的时候就自动调用析构和释放内容更加方便。

-- 4. 需要等待 Loop函数中关于该channel的所有函数都运行完，才能删除这个channel.所有需要提供延时删除的接口，外部需要删除的话先把要删除的socket句柄放到延时删除队列。

-- 5. 需要注意的是需要先调用 epoll.del 函数，然后才能调用socket析构。最后可以删除channel.

-- 6. TcpConnection::HandleRead, TcpConnection::HandleWrite, TcpConnection::HandleError 由自身的 std::shared_ptr 调用或者这些函数内部函数 自身的std::shared_ptr指针。这样可以对本身的函数调用做一个保护，就算外部已经移除了这个std::shared_ptr对象，但是因为这个函数本身还有所以引用计算没有变成0，是知道这个函数调用结束后才会移除这个对象。


## 测试
-- 一切正常。
