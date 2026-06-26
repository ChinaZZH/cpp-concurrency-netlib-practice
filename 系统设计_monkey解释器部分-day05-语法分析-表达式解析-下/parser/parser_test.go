package parser

import (
	"monkey/ast"
	"monkey/lexer"
	"monkey/token"
	"strconv"
	"testing"
)

// ============================================================
// 辅助函数
// ============================================================
// collectTokens 从词法分析器收集所有 Token
func collectTokens(l *lexer.Lexer) []token.Token {
	var allTokens []token.Token
	for {
		curToken := l.NextToken()
		allTokens = append(allTokens, curToken)
		if token.EOF == curToken.Type {
			break
		}
	}

	return allTokens
}

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

// testIntegerLiteral 验证整数字面量
func testIntegerLiteral(t *testing.T, exp ast.Expression, value int64) bool {
	integ, ok := exp.(*ast.IntegerLiteral)
	if !ok {
		t.Errorf("exp not *ast.IntegerLiteral, got=%T", exp)
		return false
	}

	if integ.Value != value {
		t.Errorf("integ.Value not %d, got=%d", value, integ.Value)
		return false
	}

	integValue, error := strconv.ParseInt(integ.Token.Literal, 0, 64)
	if nil != error {
		t.Errorf("integ.Token.Literal not integer, got=%q", integ.Token.Literal)
		return false
	}

	if integValue != value {
		t.Errorf("integ.Token.Literal not %d, got=%d", value, integValue)
		return false
	}

	return true
}

// testIdentifier 验证标识符
func testIdentifier(t *testing.T, exp ast.Expression, value string) bool {
	identifier, ok := exp.(*ast.Identifier)
	if !ok {
		t.Errorf("exp not *ast.Identifier, got=%T", exp)
		return false
	}

	if identifier.Value != value {
		t.Errorf("Identifier.Value  not %s, got=%s", value, identifier.Value)
		return false
	}

	if identifier.TokenLiteral() != value {
		t.Errorf("identifier.TokenLiteral()  not %s, got=%s", value, identifier.TokenLiteral())
		return false
	}

	return true
}

// testLiteralExpression 通用字面量测试
func testLiteralExpression(t *testing.T, exp ast.Expression, expected interface{}) bool {
	switch v := expected.(type) {
	case int:
		return testIntegerLiteral(t, exp, int64(v))
	case int64:
		return testIntegerLiteral(t, exp, v)
	case string:
		return testIdentifier(t, exp, v)
	}
	t.Errorf("type of exp not handled. got=%T", exp)
	return false
}

// testInfixExpression 验证中缀表达式
func testInfixExpression(t *testing.T, exp ast.Expression, left interface{}, operator string, right interface{}) bool {
	opExp, ok := exp.(*ast.InfixExpression)
	if !ok {
		t.Errorf("exp is not ast.InfixExpression. got=%T(%s)", exp, exp)
		return false
	}

	if !testLiteralExpression(t, opExp.Left, left) {
		return false
	}

	if opExp.Operator != operator {
		t.Errorf("exp.Operator is not '%s'. got=%q", operator, opExp.Operator)
		return false
	}

	if !testLiteralExpression(t, opExp.Right, right) {
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
	allTokens := collectTokens(l)
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
	allTokens := collectTokens(l)
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

// TestIdentifierExpression 测试标识符表达式
func TestIdentifierExpression(t *testing.T) {
	input := "foobar;"
	l := lexer.New(input)
	allTokens := collectTokens(l)
	p := New(allTokens)
	program := p.ParseProgram()
	checkParserErrors(t, p)

	if len(program.StatementList) != 1 {
		t.Fatalf("program has not enough statements. got=%d", len(program.StatementList))
	}

	stmt, ok := program.StatementList[0].(*ast.ExpressionStatement)
	if !ok {
		t.Fatalf("program.Statements[0] is not ast.ExpressionStatement. got=%T", program.StatementList[0])
	}

	testIdentifier(t, stmt.Expression, "foobar")
}

// TestIntegerLiteralExpression 测试整数字面量表达式
func TestIntegerLiteralExpression(t *testing.T) {
	input := "5;"
	l := lexer.New(input)
	allTokens := collectTokens(l)
	p := New(allTokens)
	program := p.ParseProgram()
	checkParserErrors(t, p)

	if len(program.StatementList) != 1 {
		t.Fatalf("program has not enough statements. got=%d", len(program.StatementList))
	}

	stmt, ok := program.StatementList[0].(*ast.ExpressionStatement)
	if !ok {
		t.Fatalf("program.Statements[0] is not ast.ExpressionStatement. got=%T", program.StatementList[0])
	}

	testIntegerLiteral(t, stmt.Expression, 5)
}

// TestParsingPrefixExpressions 测试前缀表达式
func TestParsingPrefixExpressions(t *testing.T) {
	prefixTest := []struct {
		input    string
		operator string
		value    int64
	}{
		{"!5;", "!", 5},
		{"-15", "-", 15},
	}

	for _, tt := range prefixTest {
		l := lexer.New(tt.input)
		allTokens := collectTokens(l)
		p := New(allTokens)
		program := p.ParseProgram()
		checkParserErrors(t, p)

		if len(program.StatementList) != 1 {
			t.Fatalf("program has not enough statements. got=%d", len(program.StatementList))
		}

		stmt, ok := program.StatementList[0].(*ast.ExpressionStatement)
		if !ok {
			t.Fatalf("program.Statements[0] is not ast.ExpressionStatement. got=%T", program.StatementList[0])
		}

		exp, ok := stmt.Expression.(*ast.PrefixExpression)
		if !ok {
			t.Fatalf("stmt is not ast.PrefixExpression. got=%T", stmt.Expression)
		}

		if exp.Operator != tt.operator {
			t.Errorf("exp.Operator is not '%s'. got=%s", tt.operator, exp.Operator)
		}

		testIntegerLiteral(t, exp.Right, tt.value)
	}
}

// TestParsingInfixExpressions 测试中缀表达式
func TestParsingInfixExpressions(t *testing.T) {
	infixTest := []struct {
		input      string
		leftValue  int64
		operator   string
		rightValue int64
	}{
		{"5 + 5;", 5, "+", 5},
		{"5 - 5;", 5, "-", 5},
		{"5 * 5;", 5, "*", 5},
		{"5 / 5;", 5, "/", 5},
		{"5 > 5;", 5, ">", 5},
		{"5 < 5;", 5, "<", 5},
		{"5 == 5;", 5, "==", 5},
		{"5 != 4;", 5, "!=", 4},
	}

	for _, tt := range infixTest {
		l := lexer.New(tt.input)
		allTokens := collectTokens(l)
		p := New(allTokens)
		program := p.ParseProgram()
		checkParserErrors(t, p)

		if len(program.StatementList) != 1 {
			t.Fatalf("program has not enough statements. got=%d", len(program.StatementList))
		}

		stmt, ok := program.StatementList[0].(*ast.ExpressionStatement)
		if !ok {
			t.Fatalf("program.Statements[0] is not ast.ExpressionStatement. got=%T", program.StatementList[0])
		}

		exp, ok := stmt.Expression.(*ast.InfixExpression)
		if !ok {
			t.Fatalf("stmt is not ast.InfixExpression. got=%T", stmt.Expression)
		}

		if exp.Operator != tt.operator {
			t.Errorf("exp.Operator is not '%s'. got=%s", tt.operator, exp.Operator)
		}

		testIntegerLiteral(t, exp.Left, tt.leftValue)
		testIntegerLiteral(t, exp.Right, tt.rightValue)
		testInfixExpression(t, stmt.Expression, tt.leftValue, tt.operator, tt.rightValue)
	}
}

// TestOperatorPrecedenceParsing 测试运算符优先级
func TestOperatorPrecedenceParsing(t *testing.T) {
	tests := []struct {
		input    string
		expected string
	}{
		{"-a * b", "((-a) * b)"},
		{"!-a", "(!(-a))"},
		{"a + b + c", "((a + b) + c)"},
		{"a + b - c", "((a + b) - c)"},
		{"a * b * c", "((a * b) * c)"},
		{"a * b / c", "((a * b) / c)"},
		{"a + b * c + d / e - f", "(((a + (b * c)) + (d / e)) - f)"},
		{"3 + 4; -5 * 5", "(3 + 4)((-5) * 5)"},
		{"5 > 4 == 3 < 4", "((5 > 4) == (3 < 4))"},
		{"5 < 4 != 3 > 4", "((5 < 4) != (3 > 4))"},
		{"3 + 4 * 5 == 3 * 1 + 4 * 5", "((3 + (4 * 5)) == ((3 * 1) + (4 * 5)))"},
	}

	for _, tt := range tests {
		l := lexer.New(tt.input)
		allTokens := collectTokens(l)
		p := New(allTokens)
		program := p.ParseProgram()
		checkParserErrors(t, p)

		actual := program.String()
		if actual != tt.expected {
			t.Errorf("expected=%q, got=%q", tt.expected, actual)
		}
	}
}
