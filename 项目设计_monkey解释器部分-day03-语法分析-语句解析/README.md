markdown

# Day03： monkey解释器 语法分析之语句的解析

## 核心收获

-- 1. 定义语法解析器。根据token流来一个一个token去做语法分析，再根据ast各个语句和表达式的结构的定义就可以知道各个语句和表达式是怎么做语法分析的了。
```go
// Parser 结构体
type Parser struct {
	tokens    []token.Token
	pos       int         // 当前读取位置（指向 peekToken）
	curToken  token.Token // 当前正在处理的 Token
	peekToken token.Token // 下一个 Token（预读）
	errors    []string    // 错误信息
}

```

-- 2. ParseProgram。 解析整个程序，生成ast的根节点。整个程序的语句都是ast根节点下面并列的语句子节点。

-- 3. parseStatement 根据语句的类型去做语句的语法分析。


## 测试
-- 一切正常。


