package parser

import (
	"monkey/ast"
	"monkey/lexer"
	"monkey/token"
	"testing"
)

// checkParserErrors 检查 Parser 是否有错误，如果有则测试失败
func checkParserErrors(t *testing.T, p *Parser) {
	errors := p.Errors()
	if len(errors) == 0 {
		return
	}

	t.Errorf("parser has %d errors", len(errors))
	for _, msg := range errors {
		t.Errorf("parser error: %q", msg)
	}

	t.FailNow()
}

// testLetStatement 辅助函数，验证 let 语句的标识符是否正确
func testLetStatement(t *testing.T, s ast.Statement, name string) bool {
	if s.TokenLiteral() != "let" {
		t.Errorf("s.TokenLiteral not 'let', got=%q", s.TokenLiteral())
		return false
	}

	letStmt, ok := s.(*ast.LetStatement)
	if !ok {
		t.Errorf("s not *ast.LetStatement, got=%T", s)
		return false
	}

	if letStmt.Name.Value != name {
		t.Errorf("LetStmt.Name.Value not %s, got=%s", name, letStmt.Name.Value)
		return false
	}

	if letStmt.Name.TokenLiteral() != name {
		t.Errorf("LetStmt.Name.TokenLiteral() not %s, got=%s", name, letStmt.Name.TokenLiteral())
		return false
	}

	return true
}

// TestLetStatement 测试let语句解析
func TestLetStatement(t *testing.T) {
	input := `
	let x = 5;
	let y = 10;
	let foobar = 838383;
	`

	l := lexer.New(input)
	allTokens := []token.Token{}
	for {
		curToken := l.NextToken()
		allTokens = append(allTokens, curToken)
		if token.EOF == curToken.Type {
			break
		}
	}

	p := New(allTokens)
	program := p.ParseProgram()
	checkParserErrors(t, p)

	if nil == program {
		t.Fatalf("parseProgram() returned nil")
	}

	if 3 != len(program.StatementList) {
		t.Fatalf("program.Statements does not contain 3 statement. got = %d", len(program.StatementList))
	}

	expectedIdentifiers := []string{"x", "y", "foobar"}
	for i, tt := range expectedIdentifiers {
		stmt := program.StatementList[i]
		if !testLetStatement(t, stmt, tt) {
			return
		}
	}

}

// TestReturnStatements 测试 return 语句解析
func TestReturnStatement(t *testing.T) {
	input := `
	return 5;
	return 10;
	return 993322;
	`

	l := lexer.New(input)
	allTokens := []token.Token{}
	for {
		curToken := l.NextToken()
		allTokens = append(allTokens, curToken)
		if token.EOF == curToken.Type {
			break
		}
	}

	p := New(allTokens)
	program := p.ParseProgram()
	checkParserErrors(t, p)

	if nil == program {
		t.Fatalf("parseProgram() returned nil")
	}

	if 3 != len(program.StatementList) {
		t.Fatalf("program.Statements does not contain 3 statement. got = %d", len(program.StatementList))
	}

	for _, stmt := range program.StatementList {
		returnStmt, ok := stmt.(*ast.ReturnStatement)
		if !ok || nil == returnStmt {
			t.Errorf("s not *ast.ReturnStatement, got=%T", stmt)
			continue
		}

		if returnStmt.TokenLiteral() != "return" {
			t.Errorf("returnStmt .TokenLiteral not 'return', got=%q", returnStmt.TokenLiteral())
		}
	}
}
