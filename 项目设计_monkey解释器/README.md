markdown

# monkey解释器 

## 核心收获

-- 1. 词法分析。

-- 2. 语法分析，定义抽象语法树ast的结点的结构。

-- 3. 语法分析，语句解析

-- 4. 语法分析，表达式的解析

-- 5. 定义对象系统和求值器求值流程

-- 6. 定义环境和调用函数，定义函数，标识符绑定，标识符取值的求值器实现。

-- 7. 功能扩展，支持字符串，哈希表，数组，以及数组和哈希表的索引取值的词法分析，抽象语法树的结点结构定义， 语法分析，还有求值。

-- 8. 支持go语言的内置函数， 增加REPL。

## 整个程序的核心

-- 1. 语法分析
```go
// Parser 结构体
type Parser struct {
	tokens    []token.Token
	pos       int         // 当前读取位置（指向 peekToken）
	curToken  token.Token // 当前正在处理的 Token
	peekToken token.Token // 下一个 Token（预读）
	errors    []string    // 错误信息
}

// 这个是整个语法分析的核心和主框架
func (p *Parser) parseExpression(precedence int) ast.Expression {
	// 先从前缀表达式解析函数表中找对应的前缀表达式解析函数
	prefixFn, ok := prefixParseFns[p.curToken.Type]
	if !ok || nil == prefixFn {
		p.noPrefixParseFnError(p.curToken.Type)
		return nil
	}

	// 找到了前缀表达式解析函数，调用解析函数做语法分析，然后返回对应的表达式
	leftExp := prefixFn(p)

	// 循环判断：当前优先级低于后面运算符优先级时，继续深入解析右子树；
	// 否则右子树闭合，返回当前表达式，让上层处理后续（不高于当前优先级的）运算符。
	for !p.peekTokenIs(token.SEMICOLON) && precedence < p.peekPrecedence() {
		infixFn, ok := infixParseFns[p.peekToken.Type]
		if !ok || nil == infixFn {
			return nil
		}

		// 下一个运算符优先级高于当前优先级，说明它应该成为当前表达式的父辈节点；
		// 因此进入下一层解析，让 infixFn 处理这个更高优先级的运算。
		leftExp = infixFn(p, leftExp)
	}

	return leftExp
}
```

-- 2. 求值器
```go
func evalProgram(program *ast.Program) object.Object {
	var result object.Object
	for _, statement := range program.StatementList {
		result = Eval(statement)
		returnVal, ok := result.(*object.ReturnValue)
		if ok && nil != returnVal {
			return returnVal.Value
		}
	}

	return result
}
```