markdown

# Day43： 网络库扩展完善 增加定时器

## 核心收获
-- 1.  线程池函数变量HttpContext context修改为thread_local HttpContext context 减少构造析构。

-- 2. 利用timerFd定时器句柄来实现定时器功能。单次定时器。

-- 3. ::timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC) 创建定时器句柄。

-- 4.  ::timerfd_settime 新增新的定时事件。



## 代码


## 测试
-- 一切正常。
