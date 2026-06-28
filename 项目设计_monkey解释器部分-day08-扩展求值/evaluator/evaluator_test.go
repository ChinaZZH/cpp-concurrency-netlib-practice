package evaluator

import (
	"monkey/lexer"
	"monkey/object"
	"monkey/parser"
	"testing"
)

func TestEvalIntegerExpression(t *testing.T) {
	tests := []struct {
		input    string
		expected int64
	}{
		{"5", 5},
		{"10", 10},
		{"-5", -5},
		{"-10", -10},
	}

	for index, tt := range tests {
		resultObj := testEval(t, index, tt.input)
		testIntegerObject(t, resultObj, tt.expected)
	}
}

func TestEvalBooleanExpression(t *testing.T) {
	tests := []struct {
		input    string
		expected bool
	}{
		{"true", true},
		{"false", false},
		{"1 < 2", true},
		{"1 > 2", false},
		{"1 < 1", false},
		{"1 > 1", false},
		{"1 == 1", true},
		{"1 != 1", false},
		{"1 == 2", false},
		{"1 != 2", true},
		{"true == true", true},
		{"false == false", true},
		{"true == false", false},
		{"true != false", true},
		{"false != true", true},
		{"(1 < 2) == true", true},
		{"(1 < 2) == false", false},
		{"(1 > 2) == true", false},
		{"(1 > 2) == false", true},
	}

	for index, tt := range tests {
		resultObj := testEval(t, index, tt.input)
		testBooleanObject(t, resultObj, tt.expected)
	}
}

func TestBangOperator(t *testing.T) {
	tests := []struct {
		input    string
		expected bool
	}{
		{"!true", false},
		{"!false", true},
		{"!5", false},
		{"!!true", true},
		{"!!false", false},
		{"!!5", true},
		{"!(3 > 2)", false},
		{"!(3 == 2)", true},
	}

	for index, tt := range tests {
		resultObj := testEval(t, index, tt.input)
		testBooleanObject(t, resultObj, tt.expected)
	}
}

func TestIfElseExpression(t *testing.T) {
	tests := []struct {
		input    string
		expected interface{}
	}{
		{"if (true) { 10 }", 10},
		{"if (false) { 10 }", nil},
		{"if (1) { 10 }", 10},
		{"if (1 < 2) { 10 }", 10},
		{"if (1 > 2) { 10 }", nil},
		{"if (1 > 2) { 10 } else { 20 }", 20},
		{"if (1 < 2) { 10 } else { 20 }", 10},
	}

	for index, tt := range tests {
		resultObj := testEval(t, index, tt.input)
		integer, ok := tt.expected.(int)
		if ok {
			testIntegerObject(t, resultObj, int64(integer))
		} else {
			testNullObject(t, resultObj)
		}
	}
}

func TestReturnStatement(t *testing.T) {
	tests := []struct {
		input    string
		expected int64
	}{
		{"return 10;", 10},
		{"return 10; 9;", 10},
		{"return 2 * 5; 9;", 10},
		{"9; return 2 * 5; 9;", 10},
		{`
        if (10 > 1) {
            if (10 > 1) {
                return 10;
            }
            return 1;
        }
        `, 10},
	}

	for index, tt := range tests {
		resultObj := testEval(t, index, tt.input)
		testIntegerObject(t, resultObj, tt.expected)
	}
}

func TestLetStatement(t *testing.T) {
	tests := []struct {
		input    string
		expected int64
	}{
		{"let a = 5; a;", 5},
		{"let a = 5 * 5; a;", 25},
		{"let a = 5; let b = a; b;", 5},
		{"let a = 5; let b = a; let c = a + b + 5; c;", 15},
	}

	for index, tt := range tests {
		resultObj := testEval(t, index, tt.input)
		testIntegerObject(t, resultObj, tt.expected)
	}
}

func TestFunctionApplication(t *testing.T) {
	tests := []struct {
		input    string
		expected int64
	}{
		{"let identity = fn(x) { x; }; identity(5);", 5},
		{"let identity = fn(x) { return x; }; identity(5);", 5},
		{"let double = fn(x) { x * 2; }; double(5);", 10},
		{"let add = fn(x, y) { x + y; }; add(5, 5);", 10},
		{"let add = fn(x, y) { x + y; }; add(5 + 5, add(5, 5));", 20},
		{"fn(x) { x; }(5)", 5},
		{"let identity = fn(x) { return x; 3; 45;}; identity(5);", 5},
	}

	for index, tt := range tests {
		resultObj := testEval(t, index, tt.input)
		testIntegerObject(t, resultObj, tt.expected)
	}
}

func TestClosure(t *testing.T) {
	input := `
        let newAdder = fn(x) {
            fn(y) { x + y };
        };
        let addTwo = newAdder(2);
        addTwo(3);
    `

	evaluated := testEval(t, 0, input)
	testIntegerObject(t, evaluated, 5)
}

func TestStringLiteral(t *testing.T) {
	input := `"hello world";`
	evaluated := testEval(t, 0, input)
	testStringObject(t, evaluated, "hello world")
}

func TestStringConcatenation(t *testing.T) {
	input := `"hello" + " " + "world";`
	evaluated := testEval(t, 0, input)
	testStringObject(t, evaluated, "hello world")
}

func TestArrayLiterals(t *testing.T) {
	input := `[1, 2 * 2, 3 + 3]`
	evaluated := testEval(t, 0, input)
	result, ok := evaluated.(*object.Array)
	if !ok {
		t.Fatalf("object is not Array. got=%T (%+v)", evaluated, evaluated)
	}

	if len(result.Element) != 3 {
		t.Fatalf("array has wrong number of elements. got=%d", len(result.Element))
	}

	testIntegerObject(t, result.Element[0], 1)
	testIntegerObject(t, result.Element[1], 4)
	testIntegerObject(t, result.Element[2], 6)
}

func TestArrayIndexLiterals(t *testing.T) {
	tests := []struct {
		input    string
		expected interface{}
	}{
		{"[1, 2, 3][0]", 1},
		{"[1, 2, 3][1]", 2},
		{"[1, 2, 3][2]", 3},
		{"let i = 0; [1][i];", 1},
		{"[1, 2, 3][1 + 1];", 3},
		{"let myArray = [1, 2, 3]; myArray[2];", 3},
		{"let myArray = [1, 2, 3]; myArray[0] + myArray[1] + myArray[2];", 6},
		{"let myArray = [1, 2, 3]; let i = myArray[0]; myArray[i];", 2},
		{"[1, 2, 3][3]", nil},
		{"[1, 2, 3][-1]", nil},
	}

	for _, tt := range tests {
		evaluated := testEval(t, 0, tt.input)
		//result, ok := evaluated.(*object.Integer)
		integer, ok := tt.expected.(int)
		if ok {
			testIntegerObject(t, evaluated, int64(integer))
		} else {
			testNullObject(t, evaluated)
		}
	}
}

func TestHashLiterals(t *testing.T) {
	input := `{"one": 1, "two": 2}`
	evaluated := testEval(t, 0, input)
	hash, ok := evaluated.(*object.Hash)
	if !ok {
		t.Fatalf("expected Hash, got %T", evaluated)
	}
	if len(hash.Pairs) != 2 {
		t.Errorf("expected 2 pairs, got %d", len(hash.Pairs))
	}
}

func TestHashIndexExpressions(t *testing.T) {
	tests := []struct {
		input    string
		expected interface{}
	}{
		{`{"key": 1}["key"]`, 1},
		{`let h = {"key": 2}; h["key"];`, 2},
		{`{"key": 3}["missing"]`, nil},
		{`let h = {"a": 1, "b": 2}; h["a"] + h["b"];`, 3},
		{`let h = {true: 10, false: 20}; h[true];`, 10},
		{`let h = {1: "one", 2: "two"}; h[1];`, "one"},
	}

	for index, tt := range tests {
		evaluatedObj := testEval(t, index, tt.input)
		switch expected := tt.expected.(type) {
		case int:
			testIntegerObject(t, evaluatedObj, int64(expected))
		case string:
			testStringObject(t, evaluatedObj, expected)
		case nil:
			testNullObject(t, evaluatedObj)
		}
	}
}

// ============================================================
// 辅助函数
// ============================================================
func testEval(t *testing.T, index int, input string) object.Object {
	l := lexer.New(input)
	tokens := l.Tokens()

	p := parser.New(tokens)
	program := p.ParseProgram()

	if len(p.Errors()) != 0 {
		t.Fatalf("index %d, parse errors: %v", index, p.Errors())
	}

	env := object.NewEnvironment()
	evalResult := Eval(program, env)
	return evalResult
}

func testIntegerObject(t *testing.T, obj object.Object, expected int64) bool {
	result, ok := obj.(*object.Integer)
	if !ok || nil == result {
		t.Fatalf("objcet is not integer. got = %T (%+v)", obj, obj)
		return false
	}

	if result.Value != expected {
		t.Fatalf("objcet has wrong value got = %d, want=%d", result.Value, expected)
		return false
	}

	return true
}

func testBooleanObject(t *testing.T, obj object.Object, expected bool) bool {
	result, ok := obj.(*object.Boolean)
	if !ok || nil == result {
		t.Fatalf("objcet is not Boolean. got = %T (%+v)", obj, obj)
		return false
	}

	if result.Value != expected {
		t.Fatalf("objcet has wrong value got = %t, want=%t", result.Value, expected)
		return false
	}

	return true
}

func testNullObject(t *testing.T, obj object.Object) bool {
	result, ok := obj.(*object.Null)
	if !ok || nil == result {
		t.Fatalf("objcet is not Null. got = %T (%+v)", obj, obj)
		return false
	}

	return true
}

func testStringObject(t *testing.T, obj object.Object, expected string) bool {
	result, ok := obj.(*object.String)
	if !ok || nil == result {
		t.Fatalf("objcet is not String. got = %T (%+v)", obj, obj)
		return false
	}

	if result.Value != expected {
		t.Fatalf("String has wrong value. got=%q", result.Value)
		return false
	}

	return true
}
