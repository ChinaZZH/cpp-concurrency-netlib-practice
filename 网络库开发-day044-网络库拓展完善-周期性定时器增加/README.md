markdown

# Day44： 网络库扩展完善 周期性定时器的增加

## 核心收获
-- 1.  通过递归调用runEvery 来实现周期定时器的调用。

-- 2. RunEvery 通过调用 RunAfter 在到期时间到的时候先执行对应的funCb，然后再递归调用RunEvery.



## 代码


## 测试
-- 一切正常。
