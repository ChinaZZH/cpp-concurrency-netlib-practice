markdown

# Day22： 网络库day005 封装TcpConnection

## 核心收获
-- 1. 封装了TcpConnection,TcpConnection是集成了 Channel和 ClientSocket. 

-- 2. 把 ClientSocket相关的读写数据操作和关闭连接操作也一并集成 到 TcpConnnection.

-- 3. 为了TcpConnection对象的 std::shared_ptr<TcpConnection>的this指针，则需要集成std::enable_shared_from_this<TcpConnection>。

-- 4. 返回自身的 std::shared_ptr<TcpConnection>的this指针 需要调用 shared_from_this();

-- 5. 通过std::bind(&function_x， xx1); 实现有参函数转化为无参函数。

-- 6. ClientSocekt具体的事件调用呈现栈式结构。该开始是 从EventLoop 中 通过Wait函数获取到 activeChannel. 

    然后activeChannel 根据revents进行二进制位运算进行出到底是读写事件还是异常事件，然后通过 activeChannel->xxCallBack_ 执行回调函数。

    因为TcpConnection拥有activeChannel所有权，所以xxCallBack_是TcpConnection在构造的时候就注册的。执行的是HandleRead, HandWrite,
     
    HandError等函数。并且目前TcpConnection是main函数左右，所以main函数注册了Tcp关于收到消息和关闭的函数。

## 流程图
   main[有TcpConnection所有权]->注册TcpConnection中的MessageCallBack和CloseCallBack函数

   TcpConnection[有Channel所有权]->注册Channel中的readCallBack_和writeCallBack_函数和errorCallBack_

   main ---> EventLoop中的Loop()函数 ---> Channel中的HandleEvent ---> 根据掩码调用 Channel中errorCallBack_，
   
   readCallBack_，writeCallBack_ ---> 等同与 TcpConnection的 HandleError， HandleRead， HandleWrite 
   
   ----> 然后调用 Tcp的 MessageCallBack 或者 CloseCallBack
 

## 代码
-- TcpConnection.cpp

-- TcpConnection.h

## 测试
-- 一切正常。
