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

	evalResult := Eval(program)
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
