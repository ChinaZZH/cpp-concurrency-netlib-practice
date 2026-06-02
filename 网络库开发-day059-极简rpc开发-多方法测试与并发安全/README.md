markdown

# Day59： 网络库扩展完善 多方法测试与并发安全

## 核心收获

-- 1. TcpClient 是否建立连接需要判断当前是否有可写事件，可写事件发生后需要先移除旧的channel创建新的channel.

-- 2. TcpClient执行移除channel的时候也是走下一帧执行。在HandleWrite移除channel之后应该执行创建新 连接的逻辑 HandleNewConnection.

-- 3. RpcClient发送消息的时候应该走eventloop中的loop线程上(和收消息同一个线程同一个channel)，所以通过投递函数RunInLoop让发消息都走在eventloop的loop线程上。

-- 4. rpc处理函数除了add方法，还增加login和echo方法，并且依次调用测试，字符串参数和模板化参数都得调用测试。

-- 5. 测试rpcClient 10个线程分别调用100次add函数, 发现没有问题运行正常稳定。

-- 6. 运行客户端程序 valgrind --leak-check=full ./rpc_client  查看是否有内存泄漏。目前没有内存泄漏，但是有内存错误报错 
-- ERROR SUMMARY: 20 errors from 2 contexts (suppressed: 0 from 0)  

-- 7. 排查报错通用命令运行客户端程序   valgrind --leak-check=full -s ./rpc_client。 定义到时channel过早移除的原因，则tcpClient跟serverConnection一样也是延时删除。


## 代码


## 测试
-- 一切正常。
