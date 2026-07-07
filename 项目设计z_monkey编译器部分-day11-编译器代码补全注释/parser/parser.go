package parser

import (
	"fmt"
	"monkey/ast"
	"monkey/token"
	"strconv"
)

// Parser 结构体
type Parser struct {
	tokens    []token.Token
	pos       int         // 当前读取位置（指向 peekToken）
	curToken  token.Token // 当前正在处理的 Token
	peekToken token.Token // 下一个 Token（预读）
	errors    []string    // 错误信息
}

// 优先级常量，解析开始默认的优先级是LOWEST最低优先级。
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

// 前缀解析函数类型
type prefixParseFn func(*Parser) ast.Expression

// 中缀解析函数类型
type infixParseFn func(*Parser, ast.Expression) ast.Expression

var prefixParseFns map[token.TokenType]prefixParseFn

var infixParseFns map[token.TokenType]infixParseFn

var precedences map[token.TokenType]int

// New 创建一个新的Parser实例
func New(tokens []token.Token) *Parser {
	p := &Parser{
		tokens: tokens,
		pos:    0,
		errors: []string{},
	}

	// 创建前缀表达式解析函数映射表 token类型与对应的表达式解析函数
	prefixParseFns = map[token.TokenType]prefixParseFn{
		token.IDENT:    (*Parser).parseIdentifier,
		token.INT:      (*Parser).parseIntegerLiteral,
		token.BANG:     (*Parser).parsePrefixExpression,
		token.MINUS:    (*Parser).parsePrefixExpression,
		token.LPAREN:   (*Parser).parseGroupedExpression,
		token.IF:       (*Parser).parseIfExpression,
		token.FUNCTION: (*Parser).parseFunctionLiteral,
		token.TRUE:     (*Parser).parseBoolean,
		token.FALSE:    (*Parser).parseBoolean,
		token.STRING:   (*Parser).parseStringLiteral,
		token.LBRACKET: (*Parser).parseArrayExpression,
		token.LBRACE:   (*Parser).parseHashLiteral,
	}

	// 创建中缀表达式解析函数映射表 token类型与对应的表达式解析函数
	infixParseFns = map[token.TokenType]infixParseFn{
		token.PLUS:     (*Parser).parseInfixExpression,
		token.MINUS:    (*Parser).parseInfixExpression,
		token.ASTERISK: (*Parser).parseInfixExpression,
		token.SLASH:    (*Parser).parseInfixExpression,
		token.LT:       (*Parser).parseInfixExpression,
		token.GT:       (*Parser).parseInfixExpression,
		token.EQ:       (*Parser).parseInfixExpression,
		token.NOT_EQ:   (*Parser).parseInfixExpression,
		token.LPAREN:   (*Parser).parseCallExpression,
		token.LBRACKET: (*Parser).parseIndexExpression,
	}

	// 创建分隔符号对应的优先级 分隔符号是语法解析的一个转折性判断
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
		token.LBRACKET: CALL, // 索引操作优先级与函数调用相同
	}
	// 读取前两个token,初始化curToken和peekToken
	p.nextToken()
	p.nextToken()
	return p
}

// nextToken 前进到下一个token
func (p *Parser) nextToken() {
	p.curToken = p.peekToken
	if p.pos < len(p.tokens) {
		p.peekToken = p.tokens[p.pos]
		p.pos += 1
	} else {
		p.peekToken = token.Token{Type: token.EOF, Literal: "\x00"}
	}
}

// curTokenIs 检查当前的Token类型是否匹配
func (p *Parser) curTokenIs(t token.TokenType) bool {
	return p.curToken.Type == t
}

// peekTokenIs 检查下一个Token类型是否匹配
func (p *Parser) peekTokenIs(t token.TokenType) bool {
	return p.peekToken.Type == t
}

// expectPeek 检查下一个 Token 是否为期望类型，如果是则前进，否则记录错误
func (p *Parser) expectPeek(t token.TokenType) bool {
	if p.peekTokenIs(t) {
		p.nextToken()
		return true
	}

	p.peekError(t)
	return false
}

// peekError 记录预期的Token错误
func (p *Parser) peekError(t token.TokenType) {
	msg := fmt.Sprintf("expected next token to be %s, got %s instead", t, p.peekToken.Type)
	p.errors = append(p.errors, msg)
}

// Errors 返回所有的解析错误
func (p *Parser) Errors() []string {
	return p.errors
}

// parseProgram 解析整个程序，生成AST根结点
func (p *Parser) ParseProgram() *ast.Program {
	program := &ast.Program{
		StatementList: []ast.Statement{},
	}

	// 循环解析整个程序的所有语句，然后把每一句语句生成的语句节点ast.Statement拼接到根节点的数组列表后面。
	// 即program.StatementList = append(program.StatementList, stmt)
	for !p.curTokenIs(token.EOF) {
		stmt := p.parseStatement()
		if nil != stmt {
			program.StatementList = append(program.StatementList, stmt)
		}

		// 语句和语句之间的分隔符;在这边跳过， parseStatement()不去消费;让总控制程序去消费;
		p.nextToken()
	}

	return program
}

// parseStatement 根据当前的token类型分发到具体的语句解析函数
// blockStatemtn由if(){}else{}或者func(){} 两个表达式控制，
// 这边主程序在大的方向上就是解析letStmt, returnStmt, expressionStmt, 三种类型的语句
func (p *Parser) parseStatement() ast.Statement {
	switch p.curToken.Type {
	case token.LET:
		return p.parseLetStatement()
	case token.RETURN:
		return p.parseReturnStatement()
	default:
		return p.parseExpressionStatement()
	}
}

// 下面就是各种语句和表达式的特定解析，同时还有一些辅助函数
// 每一个对应表达式的解析，会在函数内标明对应ast.node的定义让程序更加简单明了。
func (p *Parser) parseLetStatement() *ast.LetStatement {
	/* LetStatement 表示变量声明语句: let <identifier> = <expression>;
	   type LetStatement struct {
	   	Token token.Token // token.LET
	   	Name  *Identifier // 变量名（标识符表达式）
	   	Value Expression  // 初始值表达式
	   }
	*/

	stmt := &ast.LetStatement{
		Token: p.curToken,
	}

	if !p.expectPeek(token.IDENT) {
		return nil
	}

	stmt.Name = &ast.Identifier{Token: p.curToken, Value: p.curToken.Literal}
	if !p.expectPeek(token.ASSIGN) { // 判断下一个token是否是assign，否的话返回时候，是的话则消费掉前一个token.这个时候当前token就是assing了。
		return nil
	}

	p.nextToken() // 消费调= assign
	stmt.Value = p.parseExpression(LOWEST)
	// 这边除了可以使用 if p.peekTokenIs(token.SEMICOLON) {p.nextToken()}
	// 也可以使用 if !expectPeek(token.SEMICOLON) {return nil} 这样更具有防御性
	if p.peekTokenIs(token.SEMICOLON) {
		p.nextToken() // 消费掉前一个token.这个时候当前token就是SEMICOLON了。
	}

	return stmt
}

// 下面就是各种语句和表达式的特定解析，同时还有一些辅助函数
// 每一个对应表达式的解析，会在函数内标明对应ast.node的定义让程序更加简单明了。
func (p *Parser) parseReturnStatement() *ast.ReturnStatement {
	/* ReturnStatement 表示返回语句: return <expression>;
	   type ReturnStatement struct {
	   	Token       token.Token // token.RETURN
	   	ReturnValue Expression
	   }
	*/
	stmt := &ast.ReturnStatement{
		Token: p.curToken,
	}

	p.nextToken()
	if p.curTokenIs(token.SEMICOLON) {
		return stmt
	}

	stmt.ReturnValue = p.parseExpression(LOWEST)
	if p.peekTokenIs(token.SEMICOLON) {
		p.nextToken()
	}

	return stmt
}

func (p *Parser) parseExpressionStatement() *ast.ExpressionStatement {
	/* ExpressionStatement 表示一个仅包含表达式的语句: <expression>;
	type ExpressionStatement struct {
		Token      token.Token // 表达式的第一个 Token
		Expression Expression
	}
	*/
	stmt := &ast.ExpressionStatement{
		Token: p.curToken,
	}

	stmt.Expression = p.parseExpression(LOWEST)
	if p.peekTokenIs(token.SEMICOLON) {
		p.nextToken()
	}

	return stmt
}

// ============================================================
// 表达式解析
// ============================================================
func (p *Parser) parseIdentifier() ast.Expression {
	/* Identifier 表示标识符表达式（变量名、函数名等）
	type Identifier struct {
		Token token.Token // token.IDENT
		Value string
	}
	*/

	return &ast.Identifier{Token: p.curToken, Value: p.curToken.Literal}
}

func (p *Parser) parseIntegerLiteral() ast.Expression {
	/* IntegerLiteral 表示整数字面量
	type IntegerLiteral struct {
		Token token.Token // token.INT
		Value int64
	}
	*/
	lit := &ast.IntegerLiteral{Token: p.curToken}
	value, error := strconv.ParseInt(p.curToken.Literal, 0, 64)
	if nil != error {
		msg := fmt.Sprintf("Could not parse %q as integer", p.curToken.Literal)
		p.errors = append(p.errors, msg)
		return nil
	}

	lit.Value = value
	return lit
}

func (p *Parser) noPrefixParseFnError(t token.TokenType) {
	msg := fmt.Sprintf("no prefix parse function for %s found", t)
	p.errors = append(p.errors, msg)
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

// peekPrecedence 获取下一个 Token 的优先级
func (p *Parser) peekPrecedence() int {
	prec, ok := precedences[p.peekToken.Type]
	if ok {
		return prec
	}

	return LOWEST
}

// currPrecedence 获取当前 Token 的优先级
func (p *Parser) currPrecedence() int {
	prec, ok := precedences[p.curToken.Type]
	if ok {
		return prec
	}

	return LOWEST
}

func (p *Parser) parseInfixExpression(left ast.Expression) ast.Expression {
	/* InfixExpression 表示中缀表达式: <expression><operator><expression>
	type InfixExpression struct {
		Token    token.Token // 运算符 token，如 '+', '-', '*', '/', '<', '>', '==', '!='
		Left     Expression
		Operator string
		Right    Expression
	}
	*/
	expression := &ast.InfixExpression{
		Token:    p.curToken,
		Operator: p.curToken.Literal,
		Left:     left,
	}

	precedence := p.currPrecedence()
	p.nextToken()
	expression.Right = p.parseExpression(precedence)
	return expression
}

// parsePrefixExpression 解析前缀表达式，如 !x 或 -5
func (p *Parser) parsePrefixExpression() ast.Expression {
	/* PrefixExpression 表示前缀表达式: <operator><expression>
	type PrefixExpression struct {
		Token    token.Token // 前缀运算符 token，如 '!', '-'
		Operator string
		Right    Expression
	}
	*/
	expression := &ast.PrefixExpression{
		Token:    p.curToken,
		Operator: p.curToken.Literal,
	}

	p.nextToken()
	expression.Right = p.parseExpression(PREFIX)
	return expression
}

// parseGroupedExpression 解析括号分组表达式: ( <expression> )
func (p *Parser) parseGroupedExpression() ast.Expression {
	p.nextToken()
	exp := p.parseExpression(LOWEST)
	if !p.expectPeek(token.RPAREN) {
		return nil
	}

	return exp
}

func (p *Parser) parseBlockStatement() *ast.BlockStatement {
	/*
		BlockStatement 表示代码块: { <statements> }

		type BlockStatement struct {
			Token      token.Token // '{' token
			Statements []Statement
		}
	*/
	blockStmt := &ast.BlockStatement{
		Token:      p.curToken,
		Statements: []ast.Statement{},
	}

	p.nextToken()
	for !p.curTokenIs(token.RBRACE) && !p.curTokenIs(token.EOF) {
		stmt := p.parseStatement()
		if nil != stmt {
			blockStmt.Statements = append(blockStmt.Statements, stmt)
		}

		p.nextToken()
	}

	// 注意，这里不消费Brace，由调用者决定是否消费
	return blockStmt
}

// parseIfExpression 解析 if 表达式: if (condition) { consequence } else { alternative }
func (p *Parser) parseIfExpression() ast.Expression {
	/* IfExpression 表示条件表达式: if (<condition>) <consequence> [else <alternative>]
	type IfExpression struct {
		Token       token.Token // 'if' token
		Condition   Expression
		Consequence *BlockStatement
		Alternative *BlockStatement
	}
	*/

	expression := &ast.IfExpression{
		Token: p.curToken,
	}

	if !p.expectPeek(token.LPAREN) {
		return nil
	}

	// 当时在lparaen上，则跨国lparen
	p.nextToken()
	expression.Condition = p.parseExpression(LOWEST)
	if !p.expectPeek(token.RPAREN) {
		return nil
	}

	if !p.expectPeek(token.LBRACE) {
		return nil
	}

	expression.Consequence = p.parseBlockStatement()
	if !p.curTokenIs(token.RBRACE) {
		return nil
	}

	if p.peekTokenIs(token.ELSE) {
		p.nextToken()
		if !p.expectPeek(token.LBRACE) {
			return nil
		}

		expression.Alternative = p.parseBlockStatement()
		if !p.curTokenIs(token.RBRACE) {
			return nil
		}

	}

	return expression
}

// parseFunctionLiteral 解析函数字面量: fn(parameters) { body }
func (p *Parser) parseFunctionLiteral() ast.Expression {
	/* FunctionLiteral 表示函数字面量: fn(<params>) <block>
	type FunctionLiteral struct {
		Token      token.Token // 'fn' token
		Parameters []*Identifier
		Body       *BlockStatement
	}
	*/

	lit := &ast.FunctionLiteral{Token: p.curToken}
	if !p.expectPeek(token.LPAREN) {
		return nil
	}

	p.nextToken()
	for !p.curTokenIs(token.RPAREN) {
		expression := p.parseIdentifier()
		identifier, ok := expression.(*ast.Identifier)
		if !ok {
			return nil
		}

		lit.Parameters = append(lit.Parameters, identifier)
		p.nextToken()
		if p.curTokenIs(token.RPAREN) {
			break // 跳出循环
		}

		// 如果是逗号则吃掉一个逗号，不是逗号的话就要报错
		if p.curTokenIs(token.COMMA) {
			p.nextToken()
		} else {
			msg := fmt.Sprintf("function param next must be , or ) , got %s error", p.curToken.Type)
			p.errors = append(p.errors, msg)
			return nil
		}
	}

	if !p.expectPeek(token.LBRACE) {
		return nil
	}

	lit.Body = p.parseBlockStatement()
	if !p.curTokenIs(token.RBRACE) {
		return nil
	}

	return lit
}

func (p *Parser) parseBoolean() ast.Expression {
	return &ast.Boolean{Token: p.curToken, Value: p.curTokenIs(token.TRUE)}
}

// parseCallExpression 解析调用表达式: function(arguments)
func (p *Parser) parseCallExpression(function ast.Expression) ast.Expression {
	/*
		CallExpression 表示函数调用: <function>(<arguments>)

		type CallExpression struct {
			Token     token.Token // '(' token
			Function  Expression  // 被调用的表达式（标识符或函数字面量）
			Arguments []Expression
		}
	*/
	exp := &ast.CallExpression{
		Token:     p.curToken,
		Function:  function,
		Arguments: []ast.Expression{},
	}

	// 是否是空参数
	if p.peekTokenIs(token.RPAREN) {
		p.nextToken()
		return exp
	}

	p.nextToken() // 消费掉(
	expression := p.parseExpression(LOWEST)
	exp.Arguments = append(exp.Arguments, expression)
	for p.peekTokenIs(token.COMMA) {
		p.nextToken() // 消费掉表达式的结尾
		p.nextToken() // 消费掉token.COMMA
		expression := p.parseExpression(LOWEST)
		exp.Arguments = append(exp.Arguments, expression)
	}

	if !p.expectPeek(token.RPAREN) {
		return nil
	}

	return exp
}

func (p *Parser) parseIndexExpression(arrayName ast.Expression) ast.Expression {
	/* IndexExpression 表示索引表达式，如 arr[0] 或 hash["key"]
	type IndexExpression struct {
		Token token.Token
		Left  Expression
		Index Expression
	}
	*/
	if p.peekTokenIs(token.RBRACKET) {
		return nil
	}

	indexExp := &ast.IndexExpression{Token: p.curToken, Left: arrayName}
	p.nextToken()
	indexExp.Index = p.parseExpression(LOWEST)
	if !p.expectPeek(token.RBRACKET) {
		return nil
	}

	return indexExp
}

func (p *Parser) parseStringLiteral() ast.Expression {
	/* StringLiteral 表示字符串字面量，如 "hello"
	type StringLiteral struct {
	  	Token token.Token
	  	Value string
	  }
	*/

	return &ast.StringLiteral{Token: p.curToken, Value: p.curToken.Literal}
}

// parseArrayLiteral 解析数组字面量: [<expression>, <expression>, ...]
func (p *Parser) parseArrayExpression() ast.Expression {
	/* ArrayLiteral 表示函数调用: 表示数组字面量，如 [1, 2, 3]
	type ArrayLiteral struct {
		Token    token.Token
		Elements []Expression
	}
	*/
	array := &ast.ArrayLiteral{Token: p.curToken, Elements: []ast.Expression{}}

	// 如果下一个 token 是 ']'，表示空数组
	if p.peekTokenIs(token.RBRACKET) {
		p.nextToken() // 跳到 ']'
		return array
	}

	p.nextToken() // 跳过 '['
	tmpExpression := p.parseExpression(LOWEST)
	array.Elements = append(array.Elements, tmpExpression)

	for !p.peekTokenIs(token.RBRACKET) {
		if !p.expectPeek(token.COMMA) {
			return nil
		}

		p.nextToken()
		tmpExpression := p.parseExpression(LOWEST)
		array.Elements = append(array.Elements, tmpExpression)

	}

	if !p.expectPeek(token.RBRACKET) {
		return nil
	}

	return array
}

func (p *Parser) parseHashLiteral() ast.Expression {
	/* HashLiteral 表示哈希字面量，如 {"key": 1, "value": 2}
	type HashLiteral struct {
		Token token.Token
		Pairs map[Expression]Expression
	}
	*/
	hash := &ast.HashLiteral{Token: p.curToken, Pairs: map[ast.Expression]ast.Expression{}}
	if p.peekTokenIs(token.RBRACE) {
		p.nextToken()
		return hash
	}

	p.nextToken() //跳过{
	for true {
		key := p.parseExpression(LOWEST)
		if !p.expectPeek(token.COLON) {
			return nil
		}

		p.nextToken()
		value := p.parseExpression(LOWEST)
		hash.Pairs[key] = value

		if p.peekTokenIs(token.RBRACE) {
			break
		}

		if !p.expectPeek(token.COMMA) {
			return nil
		}

		p.nextToken() // 跳过
	}

	if !p.expectPeek(token.RBRACE) {
		return nil
	}

	return hash
}
