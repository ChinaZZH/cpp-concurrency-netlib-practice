markdown

# Day73： HTTP增强功能 keep-Alive支持

## 核心收获

-- 1. httpContext通过解析报文header的时候解析出Connection，并将其是否是专门

-- 2. 通过sendfile零拷贝替代传统的文件读写，减少数据拷贝次数和上下文切换次数来提高效率。



## 测试
-- 一切正常。


