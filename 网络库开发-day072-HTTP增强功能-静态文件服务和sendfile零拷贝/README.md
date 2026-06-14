markdown

# Day72： HTTP增强功能 实现静态文件服务和sendfile零拷贝

## 核心收获

-- 1. http增加web服务，通过sendfile零拷贝把硬盘里面的文件发送给客户端。

-- 2. 通过sendfile零拷贝替代传统的文件读写，减少数据拷贝次数和上下文切换次数



## 测试
-- 一切正常。


