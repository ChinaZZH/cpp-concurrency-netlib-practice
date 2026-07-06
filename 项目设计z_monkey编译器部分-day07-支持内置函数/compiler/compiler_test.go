package compiler

import (
	"monkey/ast"
	"monkey/code"
	"monkey/lexer"
	"monkey/parser"
	"testing"
)

func TestAddConstant(t *testing.T) {
	compiler := New()

	// 添加整数常量
	idx1 := compiler.addConstant(42)
	if idx1 != 0 {
		t.Errorf("expected first constant index 0, got %d", idx1)
	}

	idx2 := compiler.addConstant(100)
	if idx2 != 1 {
		t.Errorf("expected second constant index 1, got %d", idx2)
	}

	if len(compiler.constants) != 2 {
		t.Errorf("expected compiler.constants len 2, got %d", len(compiler.constants))
	}

	if compiler.constants[0] != 42 {
		t.Errorf("expected compiler.constants[0] is 42, got %d", compiler.constants[0])
	}

	if compiler.constants[1] != 100 {
		t.Errorf("expected compiler.constants[1] is 100, got %d", compiler.constants[1])
	}
}

func TestIntegerLiteral(t *testing.T) {
	// 构造 AST：整数字面量 5
	integerNode := &ast.IntegerLiteral{Value: 5}

	compiler := New()
	err := compiler.Compile(integerNode)
	if err != nil {
		t.Errorf("compiler error: %s", err)
	}

	// 验证字节码
	expectedInstructions := code.Make(code.OpConstant, 0)
	if len(compiler.instructions) != len(expectedInstructions) {
		t.Errorf("wrong instructions length: expected %d, got %d", len(expectedInstructions), len(compiler.instructions))
	}

	for index, instruction := range expectedInstructions {
		if instruction != compiler.instructions[index] {
			t.Errorf("wrong byte at: %d expected %d, got %d", index, instruction, compiler.instructions[index])
		}
	}

	// 验证常量池
	if len(compiler.constants) != 1 {
		t.Errorf("expected 1 constant, got %d", len(compiler.constants))
	}

	firstConstant, ok := compiler.constants[0].(int64)
	if !ok {
		t.Errorf("expected constant int64, got %t", compiler.constants[0])
	}

	if firstConstant != 5 {
		t.Errorf("expected constant value 5, got %d", firstConstant)
	}
}

func TestMultipleIntergers(t *testing.T) {
	tests := []struct {
		node       ast.Node
		expected   []byte
		integerVal int64
	}{
		{&ast.IntegerLiteral{Value: 1}, code.Make(code.OpConstant, 0), 1},
		{&ast.IntegerLiteral{Value: 2}, code.Make(code.OpConstant, 1), 2},
		{&ast.IntegerLiteral{Value: 3}, code.Make(code.OpConstant, 2), 3},
	}

	compiler := New()
	for _, tt := range tests {
		err := compiler.Compile(tt.node)
		if err != nil {
			t.Fatalf("Compiler error:%s", err)
		}
	}

	// 验证字节码长度（应为 3 条指令，共 9 字节）
	expectedLen := 3 * len(code.Make(code.OpConstant, 0))
	if expectedLen != len(compiler.instructions) {
		t.Errorf("wrong instructions length: expected %d, got %d", expectedLen, len(compiler.instructions))
	}

	for i := 0; i < 3; i += 1 {
		start_index := i * 3
		for j := 0; j < 3; j += 1 {
			if compiler.instructions[start_index+j] != tests[i].expected[j] {
				t.Errorf("wrong instructions byte: expected %d, got %d", tests[i].expected[j], compiler.instructions[start_index+j])
			}
		}

		if compiler.instructions[start_index] != byte(code.OpConstant) {

		}

		index := int(compiler.instructions[start_index+1])<<8 + int(compiler.instructions[start_index+2])
		if index != i {
			t.Errorf("wrong constant index at instruction %d: expected %d, got %d", i, i, index)
		}
	}

	if len(compiler.constants) != len(tests) {
		t.Errorf("expected %d constants, got %d", len(tests), len(compiler.constants))
	}

	for index, constant_val := range compiler.constants {
		if constant_val.(int64) != tests[index].integerVal {
			t.Errorf("constant %d value wrong: expected %d, got %v", index, tests[index].integerVal, constant_val.(int))
		}
	}
}

func TestProgramCompilation(t *testing.T) {
	program := &ast.Program{
		StatementList: []ast.Statement{
			&ast.ExpressionStatement{Expression: &ast.IntegerLiteral{Value: 5}},
			&ast.ExpressionStatement{Expression: &ast.IntegerLiteral{Value: 10}},
		},
	}

	comp := New()
	err := comp.Compile(program)
	if err != nil {
		t.Fatalf("compiler error: %s", err)
	}

	expected := append(code.Make(code.OpConstant, 0), code.Make(code.OpConstant, 1)...)
	if len(comp.instructions) != len(expected) {
		t.Fatalf("wrong instructions length: expected %d, got %d", len(expected), len(comp.instructions))
	}

	for index, expected_byte := range expected {
		if expected_byte != comp.instructions[index] {
			t.Fatalf("wrong instructions byte at index %d: expected %d, got %d", index, expected_byte, comp.instructions[index])
		}
	}

	if len(comp.constants) != 2 {
		t.Fatalf("wrong comp.constants length error: expected 2, got %d", len(comp.constants))
	}

	if val, ok := comp.constants[0].(int64); !ok || val != 5 {
		t.Fatalf("first constant error: expected 5, got %d", val)
	}

	if val, ok := comp.constants[1].(int64); !ok || val != 10 {
		t.Fatalf("second constant error: expected 10, got %d", val)
	}
}

func TestBooleanLiteral(t *testing.T) {
	tests := []struct {
		input    bool
		expected []byte
	}{
		{true, code.Make(code.OpTrue)},
		{false, code.Make(code.OpFalse)},
	}

	for _, tt := range tests {
		comp := New()
		err := comp.Compile(&ast.Boolean{Value: tt.input})
		if err != nil {
			t.Fatalf("compiler error: %s", err)
		}

		if len(comp.instructions) != len(tt.expected) {
			t.Fatalf("wrong instructions length: expected %d, got %d", len(tt.expected), len(comp.instructions))
		}

		for index, expected_byte := range tt.expected {
			if expected_byte != comp.instructions[index] {
				t.Fatalf("wrong instructions byte at index %d: expected %d, got %d", index, expected_byte, comp.instructions[index])
			}
		}
	}
}

func TestPrefixExpressionCompilation(t *testing.T) {
	tests := []struct {
		node     ast.Expression
		expected []byte
	}{
		{
			node:     &ast.PrefixExpression{Operator: "!", Right: &ast.Boolean{Value: true}},
			expected: []byte{byte(code.OpTrue), byte(code.OpBang)},
		},

		{
			node:     &ast.PrefixExpression{Operator: "-", Right: &ast.IntegerLiteral{Value: 5}},
			expected: append(code.Make(code.OpConstant, 0), code.Make(code.OpMinus)...),
		},
	}

	for index, tt := range tests {
		comp := New()
		err := comp.Compile(tt.node)
		if err != nil {
			t.Fatalf("compiler error at %d: %s", index, err)
		}

		if len(comp.instructions) != len(tt.expected) {
			t.Fatalf("wrong instructions length error at %d: expected %d, got %d", index, len(tt.expected), len(comp.instructions))
		}

		for i, expected_byte := range tt.expected {
			if expected_byte != comp.instructions[i] {
				t.Fatalf("wrong instructions byte at index %d byte index %d: expected %d, got %d", index, i, expected_byte, comp.instructions[index])
			}
		}
	}
}

func TestNodeByInfixExpression(t *testing.T) {
	tests := []struct {
		node     ast.Expression
		expected []byte
	}{
		{
			node:     &ast.InfixExpression{Left: &ast.IntegerLiteral{Value: 10}, Operator: "+", Right: &ast.IntegerLiteral{Value: 11}},
			expected: append(code.Make(code.OpConstant, 0), append(code.Make(code.OpConstant, 1), code.Make(code.OpAdd)...)...),
		},

		{
			node:     &ast.InfixExpression{Left: &ast.IntegerLiteral{Value: 10}, Operator: "-", Right: &ast.IntegerLiteral{Value: 11}},
			expected: append(code.Make(code.OpConstant, 0), append(code.Make(code.OpConstant, 1), code.Make(code.OpSub)...)...),
		},

		{
			node:     &ast.InfixExpression{Left: &ast.IntegerLiteral{Value: 10}, Operator: "*", Right: &ast.IntegerLiteral{Value: 11}},
			expected: append(code.Make(code.OpConstant, 0), append(code.Make(code.OpConstant, 1), code.Make(code.OpMul)...)...),
		},

		{
			node:     &ast.InfixExpression{Left: &ast.IntegerLiteral{Value: 10}, Operator: "/", Right: &ast.IntegerLiteral{Value: 11}},
			expected: append(code.Make(code.OpConstant, 0), append(code.Make(code.OpConstant, 1), code.Make(code.OpDiv)...)...),
		},

		{
			node:     &ast.InfixExpression{Left: &ast.IntegerLiteral{Value: 10}, Operator: ">", Right: &ast.IntegerLiteral{Value: 11}},
			expected: append(code.Make(code.OpConstant, 0), append(code.Make(code.OpConstant, 1), code.Make(code.OpGreaterThan)...)...),
		},

		{
			node:     &ast.InfixExpression{Left: &ast.IntegerLiteral{Value: 10}, Operator: "<", Right: &ast.IntegerLiteral{Value: 11}},
			expected: append(code.Make(code.OpConstant, 0), append(code.Make(code.OpConstant, 1), code.Make(code.OpLessThan)...)...),
		},

		{
			node:     &ast.InfixExpression{Left: &ast.IntegerLiteral{Value: 10}, Operator: "==", Right: &ast.IntegerLiteral{Value: 11}},
			expected: append(code.Make(code.OpConstant, 0), append(code.Make(code.OpConstant, 1), code.Make(code.OpEqual)...)...),
		},

		{
			node:     &ast.InfixExpression{Left: &ast.IntegerLiteral{Value: 10}, Operator: "!=", Right: &ast.IntegerLiteral{Value: 11}},
			expected: append(code.Make(code.OpConstant, 0), append(code.Make(code.OpConstant, 1), code.Make(code.OpNotEqual)...)...),
		},

		{
			node:     &ast.InfixExpression{Left: &ast.Boolean{Value: true}, Operator: "==", Right: &ast.IntegerLiteral{Value: 11}},
			expected: append(code.Make(code.OpTrue), append(code.Make(code.OpConstant, 0), code.Make(code.OpEqual)...)...),
		},

		{
			node:     &ast.InfixExpression{Left: &ast.IntegerLiteral{Value: 10}, Operator: "!=", Right: &ast.Boolean{Value: false}},
			expected: append(code.Make(code.OpConstant, 0), append(code.Make(code.OpFalse), code.Make(code.OpNotEqual)...)...),
		},
	}

	for index, tt := range tests {
		comp := New()
		err := comp.Compile(tt.node)
		if err != nil {
			t.Fatalf("compiler error at %d: %s", index, err)
		}

		if len(comp.instructions) != len(tt.expected) {
			t.Fatalf("wrong instructions length error at %d: expected %d, got %d", index, len(tt.expected), len(comp.instructions))
		}

		for i, expected_byte := range tt.expected {
			if expected_byte != comp.instructions[i] {
				t.Fatalf("wrong instructions byte at index %d byte index %d: expected %d, got %d", index, i, expected_byte, comp.instructions[index])
			}
		}
	}
}

func TestInfixExpressions(t *testing.T) {
	tests := []struct {
		input    string
		expected []byte
	}{
		// 算术运算
		{"5 + 3", []byte{
			byte(code.OpConstant), 0x00, 0x00, // 5
			byte(code.OpConstant), 0x00, 0x01, // 3
			byte(code.OpAdd),
		}},
		{"10 - 4", []byte{
			byte(code.OpConstant), 0x00, 0x00, // 10
			byte(code.OpConstant), 0x00, 0x01, // 4
			byte(code.OpSub),
		}},
		{"3 * 2", []byte{
			byte(code.OpConstant), 0x00, 0x00, // 3
			byte(code.OpConstant), 0x00, 0x01, // 2
			byte(code.OpMul),
		}},
		{"8 / 2", []byte{
			byte(code.OpConstant), 0x00, 0x00, // 8
			byte(code.OpConstant), 0x00, 0x01, // 2
			byte(code.OpDiv),
		}},
		// 比较运算
		{"5 > 3", []byte{
			byte(code.OpConstant), 0x00, 0x00, // 5
			byte(code.OpConstant), 0x00, 0x01, // 3
			byte(code.OpGreaterThan),
		}},
		{"5 < 3", []byte{
			byte(code.OpConstant), 0x00, 0x00, // 5
			byte(code.OpConstant), 0x00, 0x01, // 3
			byte(code.OpLessThan),
		}},
		{"5 == 5", []byte{
			byte(code.OpConstant), 0x00, 0x00, // 5
			byte(code.OpConstant), 0x00, 0x01, // 5
			byte(code.OpEqual),
		}},
		{"5 != 3", []byte{
			byte(code.OpConstant), 0x00, 0x00, // 5
			byte(code.OpConstant), 0x00, 0x01, // 3
			byte(code.OpNotEqual),
		}},
	}

	for _, tt := range tests {
		// 解析输入，生成 AST
		l := lexer.New(tt.input)
		p := parser.New(l.Tokens())
		program := p.ParseProgram()
		if len(p.Errors()) > 0 {
			t.Fatalf("parser errors: %v", p.Errors())
		}

		compiler := New()
		err := compiler.Compile(program)
		if err != nil {
			t.Fatalf("compiler error: %s", err)
		}

		// 验证字节码长度
		if len(compiler.instructions) != len(tt.expected) {
			t.Errorf("wrong instructions length: expected %d, got %d", len(tt.expected), len(compiler.instructions))
		}

		// 逐字节比较
		for i, b := range tt.expected {
			if compiler.instructions[i] != b {
				t.Errorf("wrong byte at %d: expected %d, got %d", i, b, compiler.instructions[i])
			}
		}
	}
}
