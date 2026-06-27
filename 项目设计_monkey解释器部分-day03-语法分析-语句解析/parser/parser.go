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

// New 创建一个新的Parser实例
func New(tokens []token.Token) *Parser {
	p := &Parser{
		tokens: tokens,
		pos:    0,
		errors: []string{},
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

	// 循环解析直到遇到结束标志EOF
	for !p.curTokenIs(token.EOF) {
		stmt := p.parseStatement()
		if nil != stmt {
			program.StatementList = append(program.StatementList, stmt)
		}

		p.nextToken()
	}

	return program
}

// parseStatement 根据当前的token类型分发到具体的语句解析函数
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
	for !p.curTokenIs(token.RBRACE) || !p.curTokenIs(token.EOF) {
		stmt := p.parseStatement()
		if nil != stmt {
			blockStmt.Statements = append(blockStmt.Statements, stmt)
		}

		p.nextToken()
	}

	// 注意，这里不消费Brace，由调用者决定是否消费
	return blockStmt
}

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

	if !p.expectPeek(token.ASSIGN) {
		return nil
	}

	p.nextToken()
	stmt.Value = p.parseExpression(LOWEST)
	if p.peekTokenIs(token.SEMICOLON) {
		p.nextToken()
	}

	return stmt
}

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
// 表达式解析（基础版，仅支持标识符和整数）
// ============================================================

// 优先级常量
const (
	_ int = iota
	LOWEST
	// Day3 会扩展更多优先级
)

// 前缀解析函数类型
type prefixParseFn func(*Parser) ast.Expression

var prefixParseFns = map[token.TokenType]prefixParseFn{
	token.IDENT: (*Parser).parseIdentifier,
	token.INT:   (*Parser).parseIntegerLiteral,
}

func (p *Parser) parseExpression(precedence int) ast.Expression {
	prefixFn, ok := prefixParseFns[p.curToken.Type]
	if !ok || nil == prefixFn {
		p.noPrefixParseFnError(p.curToken.Type)
		return nil
	}

	leftExp := prefixFn(p)
	// Day3 会在这里添加中缀处理循环
	return leftExp
}

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
