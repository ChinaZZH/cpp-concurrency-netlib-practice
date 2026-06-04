markdown

# Day057： 极简rpc开发 实现同步阻塞等待的tcpClient

## 核心收获

-- 1. rpcClient 实现同步阻塞等待服务端的rpc response回包。

-- 2. rpcClient 封装Connect函数到ClientSocket.

-- 3. 搭建server和client进行测试流程是否正常跑通。

-- 4. channel如果收到的是close命令的话则执行移除，不走延时下一帧移除。

-- 5. 封装tcpClient 执行connect之后监听可写事件。

-- 6. 封装tcpClient 可写事件后，移除旧的channel然后创建connection的时候再创建新的channel。

## 代码


## 测试
-- 一切正常。