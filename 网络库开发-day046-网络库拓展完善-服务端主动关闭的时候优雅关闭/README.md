markdown

# Day46： 网络库扩展完善 服务端主动优雅关闭

## 核心收获
-- 1.  main函数一开头需要signal(SIGPIPE, SIG_IGN); 当服务端已经对连接关闭写入，当再写入的时候忽略信号防止服务端崩溃。

-- 2.  修改Channel::HandleEvent(). 当revents有复合事件的时候，优先处理读事件，然后是写事件，如果是EPOLLHUP关闭全连接事件的时候调用closeCallBack_(),而不是异常函数errorCallBack().

-- 3. 服务端发送数据的时候，需要判断IsWriteClosed(). 不管是半连接关闭还是全连接关闭都不允许写入。

-- 4. listenSocket的channel注册事件除了 EPOLLIN 和 EPOLLET，增加 EPOLLRDHUP.

-- 5. 修改Shutdown() 函数

```cpp
void TcpConnection::Shutdown()
{
    if(IsClosed() || closedWrite_)
    {
        return;
    }

    if(::shutdown(fd_, SHUT_WR) < 0)
    {
        std::cerr << "shutdown error fd:=" << fd_ << std::endl;
    }
    else{
        closedWrite_ = true;
        // std::cout << "close write fd:=" << fd_ << std::endl;
    }
}
```

## 代码


## 测试
-- 一切正常。
