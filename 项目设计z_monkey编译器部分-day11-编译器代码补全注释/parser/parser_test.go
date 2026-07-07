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

// TestIfExpression 测试 if 表达式解析
func TestIfExpression(t *testing.T) {
	input := `if (x < y) { x; } else { y; }`

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

	ifExp, ok := stmt.Expression.(*ast.IfExpression)
	if !ok {
		t.Fatalf("stmt is not ast.IfExpression. got=%T", stmt.Expression)
	}

	// 验证 x  < y 是否是中缀表示式
	testInfixExpression(t, ifExp.Condition, "x", "<", "y")

	// 验证 if(1) { 2 } else { 3 } 验证2
	if len(ifExp.Consequence.Statements) != 1 {
		t.Fatalf("consequence has wrong enough statements. got=%d", len(ifExp.Consequence.Statements))
	}

	conStmt, ok := ifExp.Consequence.Statements[0].(*ast.ExpressionStatement)
	if !ok {
		t.Fatalf("ifExp.Consequence.Statements[0] is not ast.ExpressionStatement. got=%T", ifExp.Consequence.Statements[0])
	}

	testIdentifier(t, conStmt.Expression, "x")

	if ifExp.Alternative == nil {
		t.Fatalf("ifExp.Alternative is nil")
	}

	// 验证 if(1) { 2 } else { 3 } 验证 3
	if len(ifExp.Alternative.Statements) != 1 {
		t.Fatalf("alternative has wrong enough statements. got=%d", len(ifExp.Alternative.Statements))
	}

	alterStmt, ok := ifExp.Alternative.Statements[0].(*ast.ExpressionStatement)
	if !ok {
		t.Fatalf("ifExp.Alternative.Statements[0] is not ast.ExpressionStatement. got=%T", ifExp.Alternative.Statements[0])
	}

	testIdentifier(t, alterStmt.Expression, "y")
}

// TestFunctionLiteralParsing 测试函数字面量解析
func TestFunctionLiteralParsing(t *testing.T) {
	input := `fn(x, y) { x + y; }`

	l := lexer.New(input)
	allTokens := collectTokens(l)
	p := New(allTokens)
	program := p.ParseProgram()
	checkParserErrors(t, p)

	// 先看整体
	if len(program.StatementList) != 1 {
		t.Fatalf("program has not enough statements. got=%d", len(program.StatementList))
	}

	stmt, ok := program.StatementList[0].(*ast.ExpressionStatement)
	if !ok {
		t.Fatalf("program.Statements[0] is not ast.ExpressionStatement. got=%T", program.StatementList[0])
	}

	funcLit, ok := stmt.Expression.(*ast.FunctionLiteral)
	if !ok {
		t.Fatalf("stmt is not ast.FunctionLiteral. got=%T", stmt.Expression)
	}

	// 再查验param
	if len(funcLit.Parameters) != 2 {
		t.Fatalf("function literal has wrong parameters number. got=%d", len(funcLit.Parameters))
	}

	testIdentifier(t, funcLit.Parameters[0], "x")
	testIdentifier(t, funcLit.Parameters[1], "y")

	// 再验证body
	if len(funcLit.Body.Statements) != 1 {
		t.Fatalf("function body has wrong number of statements. got=%d", len(funcLit.Body.Statements))
	}

	bodyStmt, ok := funcLit.Body.Statements[0].(*ast.ExpressionStatement)
	if !ok {
		t.Fatalf("body statement is not ExpressionStatement. got=%T", funcLit.Body.Statements[0])
	}

	testInfixExpression(t, bodyStmt.Expression, "x", "+", "y")
}

// TestFunctionLiteralWithNoParams 测试无参数的函数字面量
func TestFunctionLiteralWithNoParams(t *testing.T) {
	input := `fn() { 42; }`

	l := lexer.New(input)
	tokens := collectTokens(l)
	p := New(tokens)
	program := p.ParseProgram()
	checkParserErrors(t, p)

	if len(program.StatementList) != 1 {
		t.Fatalf("program has not enough statements. got=%d", len(program.StatementList))
	}

	stmt, ok := program.StatementList[0].(*ast.ExpressionStatement)
	if !ok {
		t.Fatalf("program.Statements[0] is not ast.ExpressionStatement. got=%T", program.StatementList[0])
	}

	funcLit, ok := stmt.Expression.(*ast.FunctionLiteral)
	if !ok {
		t.Fatalf("stmt.Expression is not ast.FunctionLiteral. got=%T", stmt.Expression)
	}

	if len(funcLit.Parameters) != 0 {
		t.Fatalf("function literal has wrong number of parameters. got=%d", len(funcLit.Parameters))
	}

	// 再验证body
	if len(funcLit.Body.Statements) != 1 {
		t.Fatalf("function body has wrong number of statements. got=%d", len(funcLit.Body.Statements))
	}

	bodyStmt, ok := funcLit.Body.Statements[0].(*ast.ExpressionStatement)
	if !ok {
		t.Fatalf("body statement is not ExpressionStatement. got=%T", funcLit.Body.Statements[0])
	}

	testIntegerLiteral(t, bodyStmt.Expression, 42)
}

// TestCallExpressionParsing 测试调用表达式解析
func TestCallExpressionParsing(t *testing.T) {
	input := `add(1, 2 * 3, 4 + 5);`

	l := lexer.New(input)
	tokens := collectTokens(l)
	p := New(tokens)
	program := p.ParseProgram()
	checkParserErrors(t, p)

	if len(program.StatementList) != 1 {
		t.Fatalf("program has not enough statements. got=%d", len(program.StatementList))
	}

	stmt, ok := program.StatementList[0].(*ast.ExpressionStatement)
	if !ok {
		t.Fatalf("program.Statements[0] is not ast.ExpressionStatement. got=%T", program.StatementList[0])
	}

	callExp, ok := stmt.Expression.(*ast.CallExpression)
	if !ok {
		t.Fatalf("stmt.Expression is not ast.callExp. got=%T", stmt.Expression)
	}

	testIdentifier(t, callExp.Function, "add")

	if len(callExp.Arguments) != 3 {
		t.Fatalf("wrong number of arguments. got=%d", len(callExp.Arguments))
	}
	testLiteralExpression(t, callExp.Arguments[0], 1)
	testInfixExpression(t, callExp.Arguments[1], 2, "*", 3)
	testInfixExpression(t, callExp.Arguments[2], 4, "+", 5)
}

// TestCallExpressionWithNoArgs 测试无参数的调用表达式
func TestCallExpressionWithNoArgs(t *testing.T) {
	input := `fn() { 42; }();`

	l := lexer.New(input)
	tokens := collectTokens(l)
	p := New(tokens)
	program := p.ParseProgram()
	checkParserErrors(t, p)

	if len(program.StatementList) != 1 {
		t.Fatalf("program has not enough statements. got=%d", len(program.StatementList))
	}

	stmt, ok := program.StatementList[0].(*ast.ExpressionStatement)
	if !ok {
		t.Fatalf("program.Statements[0] is not ast.ExpressionStatement. got=%T", program.StatementList[0])
	}

	callExp, ok := stmt.Expression.(*ast.CallExpression)
	if !ok {
		t.Fatalf("stmt.Expression is not ast.CallExpression. got=%T", stmt.Expression)
	}

	// Function 应该是一个 FunctionLiteral
	_, ok = callExp.Function.(*ast.FunctionLiteral)
	if !ok {
		t.Fatalf("callExp.Function is not FunctionLiteral. got=%T", callExp.Function)
	}

	if len(callExp.Arguments) != 0 {
		t.Fatalf("wrong number of arguments. got=%d", len(callExp.Arguments))
	}
}

// TestParserErrorHandling 测试 Parser 错误报告
func TestParserErrorHandling(t *testing.T) {
	input := `
        let x 5;          // 缺少 =
        let = 10;         // 缺少标识符
    `

	l := lexer.New(input)
	tokens := collectTokens(l)
	p := New(tokens)
	p.ParseProgram()

	if len(p.Errors()) == 0 {
		t.Errorf("expected at least one error, got none")
	}
}

func TestArrayLiteralParsing(t *testing.T) {
	input := "[1, 2 * 2, 3 + 3];"
	l := lexer.New(input)
	tokens := collectTokens(l)
	p := New(tokens)
	program := p.ParseProgram()
	checkParserErrors(t, p)

	stmt := program.StatementList[0].(*ast.ExpressionStatement)
	array, ok := stmt.Expression.(*ast.ArrayLiteral)
	if !ok {
		t.Fatalf("exp not ast.ArrayLiteral. got=%T", stmt.Expression)
	}

	if len(array.Elements) != 3 {
		t.Fatalf("len(array.Elements) not 3. got=%d", len(array.Elements))
	}

	testIntegerLiteral(t, array.Elements[0], 1)
	testInfixExpression(t, array.Elements[1], 2, "*", 2)
	testInfixExpression(t, array.Elements[2], 3, "+", 3)
}

func TestIndexExpressionParsing(t *testing.T) {
	input := "myArray[1 + 1]"
	l := lexer.New(input)
	p := New(l.Tokens())
	program := p.ParseProgram()
	checkParserErrors(t, p)

	stmt := program.StatementList[0].(*ast.ExpressionStatement)
	indexExp, ok := stmt.Expression.(*ast.IndexExpression)
	if !ok {
		t.Fatalf("exp not ast.IndexExpression. got=%T", stmt.Expression)
	}
	testIdentifier(t, indexExp.Left, "myArray")
	testInfixExpression(t, indexExp.Index, 1, "+", 1)
}

func TestEmptyHash(t *testing.T) {
	input := "let empty = {};"
	l := lexer.New(input)
	tokens := collectTokens(l)
	p := New(tokens)
	program := p.ParseProgram()
	checkParserErrors(t, p)

	if len(program.StatementList) != 1 {
		t.Fatalf("program has not enough statements. got=%d", len(program.StatementList))
	}

	letStmt, ok := program.StatementList[0].(*ast.LetStatement)
	if !ok {
		t.Fatalf("stmt LetStatement. got=%T", program.StatementList[0])
	}

	if letStmt.Name.Value != "empty" {
		t.Fatalf("letStmt.Name.Value error. got=%s", letStmt.Name.Value)
	}

	if letStmt.Name.TokenLiteral() != "empty" {
		t.Fatalf("letStmt.Name.TokenLiteral() error. got=%s", letStmt.Name.TokenLiteral())
	}

	hashLt, ok := letStmt.Value.(*ast.HashLiteral)
	if !ok {
		t.Fatalf("hash no ast.HashLiteral. got=%T", letStmt.Value)
	}

	if len(hashLt.Pairs) != 0 {
		t.Fatalf("len(hash.Pairs) not 3. got=%d", len(hashLt.Pairs))
	}
}

func TestHashOneData(t *testing.T) {
	input := `let nonEmpty = {"a": 1};`
	l := lexer.New(input)
	tokens := collectTokens(l)
	p := New(tokens)
	program := p.ParseProgram()
	checkParserErrors(t, p)

	if len(program.StatementList) != 1 {
		t.Fatalf("program has not enough statements. got=%d", len(program.StatementList))
	}

	letStmt, ok := program.StatementList[0].(*ast.LetStatement)
	if !ok {
		t.Fatalf("stmt LetStatement. got=%T", program.StatementList[0])
	}

	if letStmt.Name.Value != "nonEmpty" {
		t.Fatalf("letStmt.Name.Value error. got=%s", letStmt.Name.Value)
	}

	if letStmt.Name.TokenLiteral() != "nonEmpty" {
		t.Fatalf("letStmt.Name.TokenLiteral() error. got=%s", letStmt.Name.TokenLiteral())
	}

	hashLt, ok := letStmt.Value.(*ast.HashLiteral)
	if !ok {
		t.Fatalf("hash no ast.HashLiteral. got=%T", letStmt.Value)
	}

	if len(hashLt.Pairs) != 1 {
		t.Fatalf("len(hash.Pairs) not 3. got=%d", len(hashLt.Pairs))
	}

	testHashStringKeyIntegerValue(t, hashLt.Pairs, "a", 1)
}

func TestHashParsing(t *testing.T) {
	input := `let nonEmpty = {"a": 1, "b": 2};`
	l := lexer.New(input)
	tokens := collectTokens(l)
	p := New(tokens)
	program := p.ParseProgram()
	checkParserErrors(t, p)

	if len(program.StatementList) != 1 {
		t.Fatalf("program has not enough statements. got=%d", len(program.StatementList))
	}

	letStmt, ok := program.StatementList[0].(*ast.LetStatement)
	if !ok {
		t.Fatalf("stmt LetStatement. got=%T", program.StatementList[0])
	}

	if letStmt.Name.Value != "nonEmpty" {
		t.Fatalf("letStmt.Name.Value error. got=%s", letStmt.Name.Value)
	}

	if letStmt.Name.TokenLiteral() != "nonEmpty" {
		t.Fatalf("letStmt.Name.TokenLiteral() error. got=%s", letStmt.Name.TokenLiteral())
	}

	hashLt, ok := letStmt.Value.(*ast.HashLiteral)
	if !ok {
		t.Fatalf("hash no ast.HashLiteral. got=%T", letStmt.Value)
	}

	if len(hashLt.Pairs) != 2 {
		t.Fatalf("len(hash.Pairs) not 3. got=%d", len(hashLt.Pairs))
	}

	testHashStringKeyIntegerValue(t, hashLt.Pairs, "a", 1)
	testHashStringKeyIntegerValue(t, hashLt.Pairs, "b", 2)
}

func testHashStringKeyIntegerValue(t *testing.T, Pairs map[ast.Expression]ast.Expression, key string, value int64) {
	for hashKey, hashVal := range Pairs {
		keyString, ok := hashKey.(*ast.StringLiteral)
		if !ok {
			t.Fatalf("testHashStringKeyIntegerValue key must be string. got=%t", hashKey)
		}

		if keyString.Value != key {
			continue
		}

		if keyString.TokenLiteral() != key {
			t.Fatalf("keyString.TokenLiteral() error. got=%s", keyString.TokenLiteral())
		}

		valueInteger, ok := hashVal.(*ast.IntegerLiteral)
		if !ok {
			t.Fatalf("testHashStringKeyIntegerValue value must be integer. got=%t", hashVal)
		}

		if valueInteger.Value != value {
			t.Fatalf("valueInteger.Value error. got=%d", valueInteger.Value)
		}
	}
}
