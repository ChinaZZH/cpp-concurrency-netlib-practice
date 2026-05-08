markdown

# Day29： 网络库day008 Buffer设计

## 核心收获

\-- 1. 将设计以及测试好的buffer文件集成到TcpConnection中去。

\-- 2. 用memcpy和memmove替代std::copy和std::copy\_forward可以避免 处理重叠移动，更加安全。

\-- 3. MakeSpace当空余的空间不够时，则需要开辟新的空间，则需要用size\_t newSize = std::max(buffer\_.size() \* 2, buffer\_.size() + len); 这个策略会更加安全高效，不用频繁的开辟新空间，避免频繁的数据拷贝。



## 代码

\-- Buffer.h



\-- Buffer.cpp



## 测试

\-- 一切正常。

