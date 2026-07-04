package vm

import (
	"fmt"
	"io"
	"monkey/code"
	"monkey/compiler"
	"monkey/lexer"
	"monkey/object"
	"monkey/parser"
	"os"
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

		// 字符串
		{`"hello"`, "hello"},
		{`"world"`, "world"},
		{`let s = "test"; s`, "test"},

		// 数组
		{`[1, 2, 3]`, []int64{1, 2, 3}},
		{`[1 + 2, 3 * 4, 5 - 6]`, []int64{3, 12, -1}},
		{`let arr = [1, 2, 3]; arr`, []int64{1, 2, 3}},
		//{`[1, [2, 3]]`, []int64{1, 2, 3}}, // 嵌套数组暂不深入测试

		// 数组
		{`[1, 2, 3][0]`, 1},
		{`[1, 2, 3][1]`, 2},
		{`[1, 2, 3][2]`, 3},
		{`let arr = [1, 2, 3]; arr[0]`, 1},
		{`let arr = [1, 2, 3]; arr[1 + 1]`, 3},
		{`[1, 2, 3][0] + [1, 2, 3][2]`, 4},

		// 哈希表索引
		{`{"a": 1}["a"]`, 1},
		{`{"a": 1, "b": 2}["b"]`, 2},
		{`let h = {"k": 10}; h["k"]`, 10},
		{`{"a": 1}["missing"]`, nil},
		{`{"a": 1}["a"] + {"b": 2}["b"]`, 3},
		{`let h = {"key": "value"}; h["key"]`, "value"},
		{`true`, true}, // 占位，实际测试时需区分类型

		// 内置函数
		{`len("hello")`, 5},
		{`len([1, 2, 3])`, 3},
		{`len({"a":1, "b":2})`, 2},
		{`first([10, 20, 30])`, 10},
		{`rest([1, 2, 3])`, []int64{2, 3}}, // 需支持数组比较
		{`push([1, 2], 3)`, []int64{1, 2, 3}},
		{`let a = [1]; push(a, 2); a`, []int64{1}}, // 验证不可变性

		// 闭包
		//{`let addTwo = fn(x) { fn(y) { x + y; }; }; let add = addTwo(5); add(3);`, 8},
		//{`let outer = fn(x) { fn(y) { x + y; }; }; outer(2)(3);`, 5},
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
		case string:
			testStringObject(t, result, expected)
		case []int64:
			testIntArrayObject(t, result, expected)
		case nil:
			testNullObject(t, result)
		default:
			t.Errorf("unexpected expected type: %T", expected)
		}
	}
}

/*
func TestVMFunction(t *testing.T) {
	tests := []struct {
		input    string
		expected interface{} // int64, bool, string, nil (表示 null)
	}{
		// 基本函数调用（单参数）

		{"let add = fn(x) { x + 1; }; add(5);", 6},

		{"let double = fn(x) { x * 2; }; double(5);", 10},

		// 多参数函数
		{"let add = fn(x, y) { x + y; }; add(3, 4);", 7},
		{"let sub = fn(x, y) { x - y; }; sub(10, 3);", 7},
		{"let mul = fn(x, y) { x * y; }; mul(3, 4);", 12},
		{"let div = fn(x, y) { x / y; }; div(10, 2);", 5},

		// 函数调用表达式作为参数
		{"let add = fn(x, y) { x + y; }; add(add(1, 2), 3);", 6},

		// return 语句（有返回值）
		{"let identity = fn(x) { return x; }; identity(8);", 8},
		{"let doubleReturn = fn(x) { return x * 2; }; doubleReturn(5);", 10},

		// return 语句（无返回值 → null）
		{"let returnNull = fn() { return; }; returnNull();", nil},

		// 条件语句内的函数返回值
		{"let abs = fn(x) { if (x > 0) { return x; } else { return -x; }; }; abs(-5);", 5},

		// 函数体中的变量绑定
		{"let addOne = fn(x) { let y = x + 1; return y; }; addOne(3);", 4},

		{"let add = fn(x) { x + 1; }; add(5);", 6},
		{"let add = fn(x, y) { x + y; }; add(3, 4);", 7},
		{"let identity = fn(x) { return x; }; identity(10);", 10},

		// 嵌套函数调用（非闭包）
		//{"let outer = fn(x) { fn(y) { x + y; }; }; let inner = outer(2); inner(3);", 5}, // 闭包（若暂不支持，可注释）
		// 递归
		//{"let fact = fn(n) { if (n == 1) { 1 } else { n * fact(n - 1); }; }; fact(5);", 120}, // 递归（若暂不支持，可注释）

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
		case string:
			testStringObject(t, result, expected)
		case nil:
			testNullObject(t, result)
		default:
			t.Errorf("unexpected expected type: %T", expected)
		}
	}
}
*/

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

	/*
		var out io.Writer
		out = os.Stdout
		fmt.Fprintln(out, "=====Bytecode=======")
		code.DisassembleInstructions(out, comp.ByteCode(), comp.Constants())
		fmt.Fprintln(out, "=====Constants======")
		for i, c := range comp.Constants() {
			fmt.Fprintf(out, "%d: %v\n", i, c)
			if fn, ok := c.(*object.CompiledFunction); ok {
				fmt.Fprintf(out, "   Function (params=%d, locals=%d):\n", fn.NumParams, fn.NumLocals)
				code.DisassembleInstructions(out, fn.Instructions, nil)
			}
		}
	*/

	// 执行 VM

	vm := New(comp.ByteCode(), comp.Constants())
	err = vm.Run()
	if err != nil {
		t.Fatalf("VM error: %s", err)
	}

	var out io.Writer
	out = os.Stdout
	fmt.Fprintln(out, "=====Bytecode=======")
	code.DisassembleInstructions(out, vm.run_insrtuctions, nil)

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

func testIntArrayObject(t *testing.T, obj object.Object, expected []int64) {
	result, ok := obj.(*object.Array)
	if !ok {
		t.Errorf("object is not Array. got=%T (%+v)", obj, obj)
		return
	}
	if len(result.Element) != len(expected) {
		t.Errorf("object has wrong length. got=%d, want=%d", len(result.Element), len(expected))
	}

	for idx, element := range result.Element {
		elem_obj, ok := element.(*object.Integer)
		if !ok || nil == elem_obj {
			t.Errorf("elem_obj has wrong type at %T", elem_obj)
		}

		if elem_obj.Value != expected[idx] {
			t.Errorf("object has wrong valu at %d. got=%d, want=%d", idx, elem_obj.Value, expected[idx])
		}
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

func testStringObject(t *testing.T, obj object.Object, expected string) {
	result, ok := obj.(*object.String)
	if !ok {
		t.Errorf("object is not String. got=%T (%+v)", obj, obj)
		return
	}
	if result.Value != expected {
		t.Errorf("object has wrong value. got=%s, want=%s", result.Value, expected)
	}
}
