package vm

import (
	"monkey/compiler"
	"monkey/lexer"
	"monkey/object"
	"monkey/parser"
	"testing"
)

// TestVMIntegration 虚拟机集成测试
func TestVMIntegration(t *testing.T) {
	tests := []struct {
		input    string
		expected interface{} // int64, bool, string, nil (表示 null)
	}{
		// 整数常量
		{"5", 5},
		{"10", 10},
		{"-5", -5},
		{"-10", -10},

		// 布尔常量
		{"true", true},
		{"false", false},

		// 算术运算
		{"5 + 3", 8},
		{"10 - 4", 6},
		{"3 * 2", 6},
		{"8 / 2", 4},
		{"2 + 3 * 4", 14},
		{"(2 + 3) * 4", 20},

		// 前缀运算符
		{"!true", false},
		{"!false", true},
		{"!5", false},
		{"!!true", true},
		{"!!false", false},
		{"-5", -5},
		{"-(-5)", 5},

		// 比较运算符
		{"5 > 3", true},
		{"5 < 3", false},
		{"5 == 5", true},
		{"5 != 5", false},
		{"5 == 3", false},
		{"5 != 3", true},
		{"true == true", true},
		{"true == false", false},
		{"true != false", true},

		// 条件语句 (if-else)
		{"if (true) { 10 } else { 20 }", 10},
		{"if (false) { 10 } else { 20 }", 20},
		{"if (1) { 10 } else { 20 }", 10},
		{"if (0) { 10 } else { 20 }", 10}, // Monkey 中 0 为真
		{"if (true) { 10 }", 10},
		{"if (false) { 10 }", nil}, // 无 else，条件假，返回 null
		{"if (true) { if (false) { 5 } else { 6 } } else { 7 }", 6},
		{"if (false) { if (true) { 5 } else { 6 } } else { 7 }", 7},

		// 全局变量
		{"let x = 5; x", 5},
		{"let x = 5; let y = x + 2; y", 7},
		{"let a = 1; let b = 2; a + b", 3},
		{"let x = 5; let x = 10; x", 10},

		// 综合表达式
		{"let x = 5; let y = 3; x > y", true},
		{"let x = 5; -x", -5},
		{"let flag = true; !flag", false},
		{"let x = 5 + 3; x", 8},
		{"let result = if (true) { 10 } else { 20 }; result", 10},
	}

	for _, tt := range tests {
		// 执行虚拟机并获取结果
		result := testVM(t, tt.input)

		// 根据期望类型断言
		switch expected := tt.expected.(type) {
		case int:
			testIntegerObject(t, result, int64(expected))
		case int64:
			testIntegerObject(t, result, expected)
		case bool:
			testBooleanObject(t, result, expected)
		case nil:
			testNullObject(t, result)
		default:
			t.Errorf("unexpected expected type: %T", expected)
		}
	}
}

// testVM 执行一段 Monkey 代码并返回栈顶值（或 null）
func testVM(t *testing.T, input string) object.Object {
	// 词法分析、解析
	l := lexer.New(input)
	p := parser.New(l.Tokens())
	program := p.ParseProgram()

	if len(p.Errors()) != 0 {
		t.Fatalf("parser errors: %v", p.Errors())
	}

	// 编译
	comp := compiler.New()
	err := comp.Compile(program)
	if err != nil {
		t.Fatalf("compiler error: %s", err)
	}

	// 执行 VM
	vm := New(comp.ByteCode(), comp.Constants())
	err = vm.Run()
	if err != nil {
		t.Fatalf("VM error: %s", err)
	}

	// 返回栈顶结果
	return vm.LastPoppedStackElem()
}

// ---------- 断言辅助函数 ----------

func testIntegerObject(t *testing.T, obj object.Object, expected int64) {
	result, ok := obj.(*object.Integer)
	if !ok {
		t.Errorf("object is not Integer. got=%T (%+v)", obj, obj)
		return
	}
	if result.Value != expected {
		t.Errorf("object has wrong value. got=%d, want=%d", result.Value, expected)
	}
}

func testBooleanObject(t *testing.T, obj object.Object, expected bool) {
	result, ok := obj.(*object.Boolean)
	if !ok {
		t.Errorf("object is not Boolean. got=%T (%+v)", obj, obj)
		return
	}
	if result.Value != expected {
		t.Errorf("object has wrong value. got=%t, want=%t", result.Value, expected)
	}
}

func testNullObject(t *testing.T, obj object.Object) {
	if obj != Null && obj != nil {
		t.Errorf("object is not Null. got=%T (%+v)", obj, obj)
	}
}
