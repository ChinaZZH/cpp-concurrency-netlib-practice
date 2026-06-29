markdown

# Day05： monkey解释器 语法分析之表达式解析下

## 核心收获

-- 1. 增加对括号分组，函数定义，函数调用，以及if表达式的表达式的支持。

-- 2. 将括号分组，函数定义，函数调用，以及if表达式放入不同的分组进去。

```go
prefixParseFns = map[token.TokenType]prefixParseFn{
		token.IDENT: (*Parser).parseIdentifier,
		token.INT:   (*Parser).parseIntegerLiteral,
		token.BANG:  (*Parser).parsePrefixExpression,
		token.MINUS: (*Parser).parsePrefixExpression,
		token.IDENT:    (*Parser).parseIdentifier,
		token.INT:      (*Parser).parseIntegerLiteral,
		token.BANG:     (*Parser).parsePrefixExpression,
		token.MINUS:    (*Parser).parsePrefixExpression,
		token.LPAREN:   (*Parser).parseGroupedExpression,
		token.IF:       (*Parser).parseIfExpression,
		token.FUNCTION: (*Parser).parseFunctionLiteral,
	}

	infixParseFns = map[token.TokenType]infixParseFn{
		token.GT:       (*Parser).parseInfixExpression,
		token.EQ:       (*Parser).parseInfixExpression,
		token.NOT_EQ:   (*Parser).parseInfixExpression,
		token.LPAREN:   (*Parser).parseCallExpression,
	}

```

## 测试
-- 单元测试通过。


