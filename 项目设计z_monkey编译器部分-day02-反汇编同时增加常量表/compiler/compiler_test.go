package compiler

import (
	"monkey/ast"
	"monkey/code"
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
