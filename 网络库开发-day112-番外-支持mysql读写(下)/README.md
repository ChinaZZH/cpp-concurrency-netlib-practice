markdown


# Day112： 番外   支持mysql(下)

## 核心收获

-- 1.   通过 std::promise和 std::future 完成对mysql 阻塞同步读写的接口的完成。

-- 2.   将mysql模块相关集成到game_server上。 同时支持客户端传消息到服务端完成mysql的读写。

-- 3.   大量的单元测试来验证和保证。

 -- 4.   尝试mysql预处理机制。
 
### 测试
-- 1.  测试通过。


## 测试
-- 一切正常。