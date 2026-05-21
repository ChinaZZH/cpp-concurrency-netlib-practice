markdown

# Day45： 网络库扩展完善 空闲连接超时自动关闭

## 核心收获
-- 1.  每一个TcpConnection保存一个最近活跃的时间，TcpConnection构造，OnEstablished， 还有有读写操作的时候更新活动时间为当前时间。

-- 2. 设置空闲连接超时自动关闭的超时的时间，默认为60秒。

-- 3. 利用定时器每5秒检测遍历整个connection队列是否有超过空闲时间限制的connection, 有的话进行shutdown.



## 代码


## 测试
-- 一切正常。
