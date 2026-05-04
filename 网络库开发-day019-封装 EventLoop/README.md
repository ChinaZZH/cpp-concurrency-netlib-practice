markdown

# Day19： 网络库day004 封装EventLoop

## 核心收获
-- 1.  两份管理，g_channelMap持有std::unique_ptr<Channel>，是真正的所有者。而EventLoop内部的channels_只是裸指针，用来事件分发的快速查找。两者通过 UpdateChannel和 RemoveChannel保持同步。

-- 2.  Channel 和 ClientSocket 的销毁由 g_channelMap / g_clientMap 负责， 关闭连接时，先调用 EventLoop::removeChannel（删除索引），再从 map 中 erase（释放对象。最后释放ClientSocket.

-- 3. EventLoop主要封装时间循环。

-- 4. 可以通过std::bind实现有参数的函数转化为无参函数。

-- 5. 所有权和使用权索引的分离可以多思考思考，这个是这一节最重要的内容了。

## 代码
-- EventLoop.cpp
-- EventLoop.h

## 测试
-- 一切正常。
