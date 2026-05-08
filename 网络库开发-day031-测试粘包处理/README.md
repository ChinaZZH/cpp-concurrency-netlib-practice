markdown

# Day31： 网络库day009 测试处理

## 核心收获

测试日期：2026-05-08

服务器： ./echo\_server

客户端： python test\_client.py



测试1：发送 messages = \["hello","world","from","client"]

结果：收到四行 "hello", "world", "from", "client"，正确。



测试1：发送 "hello\\nworld\\n"

结果：收到两行 "hello" 和 "world"，正确。



测试2：发送 "hel" + "lo\\nworld\\n"

结果：服务器先缓存 "hel"，收到 "lo\\n" 后解析出 "hello"，再收到 "world\\n" 解析出 "world"。



结论：Buffer 正确实现了按行解析，能够处理粘包和半包。

## 代码

\-- python test\_client.py

## 测试

\-- 一切正常。

