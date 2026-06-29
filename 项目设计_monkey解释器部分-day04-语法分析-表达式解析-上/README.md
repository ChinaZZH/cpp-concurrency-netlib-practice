markdown

# Day04： monkey解释器 语法分析之表达式解析上

## 核心收获

-- 1. 前缀表达式的语法分析函数表和中缀表达式的语法分析函数表。通过token.Type 来映射对应的函数。

```go
prefixParseFns = map[token.TokenType]prefixParseFn{
		token.IDENT: (*Parser).parseIdentifier,
		token.INT:   (*Parser).parseIntegerLiteral,
		token.BANG:  (*Parser).parsePrefixExpression,
		token.MINUS: (*Parser).parsePrefixExpression,
	}

	infixParseFns = map[token.TokenType]infixParseFn{
		token.PLUS:     (*Parser).parseInfixExpression,
		token.MINUS:    (*Parser).parseInfixExpression,
		token.ASTERISK: (*Parser).parseInfixExpression,
		token.SLASH:    (*Parser).parseInfixExpression,
		token.LT:       (*Parser).parseInfixExpression,
		token.GT:       (*Parser).parseInfixExpression,
		token.EQ:       (*Parser).parseInfixExpression,
		token.NOT_EQ:   (*Parser).parseInfixExpression,
	}

```
-- 2. 定义各个节点的优先级。
```go
// 优先级常量
const (
	_ int = iota
	LOWEST
	EQUALS      // ==
	LESSGREATER // < >
	SUM         // +
	PRODUCT     // *
	PREFIX      // -X !X
	CALL        // myFunction(x)
)

precedences = map[token.TokenType]int{
		token.EQ:       EQUALS,
		token.NOT_EQ:   EQUALS,
		token.LT:       LESSGREATER,
		token.GT:       LESSGREATER,
		token.PLUS:     SUM,
		token.MINUS:    SUM,
		token.ASTERISK: PRODUCT,
		token.SLASH:    PRODUCT,
		token.LPAREN:   CALL,
	}
```

-- 3. 这个优先级作用用来做通过一条语句token流 当前层级的ParseExpression是否结束。ParseExpression函数值整个语法分析的核心和枢纽。上接语句的实现，下接各个不同类型表达式的实现。

```go
func (p *Parser) parseExpression(precedence int) ast.Expression {
	prefixFn, ok := prefixParseFns[p.curToken.Type]
	if !ok || nil == prefixFn {
		p.noPrefixParseFnError(p.curToken.Type)
		return nil
	}

	leftExp := prefixFn(p)

	// 循环处理中缀运算符
	for !p.peekTokenIs(token.SEMICOLON) && precedence < p.peekPrecedence() {
		infixFn, ok := infixParseFns[p.peekToken.Type]
		if !ok || nil == infixFn {
			return nil
		}

		p.nextToken()
		leftExp = infixFn(p, leftExp)
	}

	return leftExp
}
```


 
-- 4. 字面量表达式的语法分析(整数字面量，bool知字面量)。这个字面量表达式是语法树的叶子结点，也是语法分析和求值的递归终止条件。

-- 5. 标识符的语法分析。这个标识符表达式是语法树的叶子结点，也是语法分析和求值的递归终止条件。


## 测试
-- 通过单元测试。


