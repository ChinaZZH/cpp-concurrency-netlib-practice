markdown

# Day60： 网络库扩展完善 网络库粘包处理优化（长度前缀拆包）

## 核心收获

-- 1. 实现rpcServer网络库粘包处理，使用长度前缀拆包

-- 2. 修复rpcClient的rpc_timeout超时问题。

-- 3. rpcClient超时问题. 在消息发送前，就需要将promise存入，而不是发送后。因为消息回包可能在存入promise之前。(发送后CPU切换直到很晚才切到存入promise逻辑)

-- 4. 使用火焰图 perf 分析CPU性能的瓶颈，发现性能瓶颈在json的拆包和解包，后期会使用protobuf来进行相对应的优化。

-- 5. 同级目录有对应的火焰图。


## 代码


## 测试
-- 一切正常。
