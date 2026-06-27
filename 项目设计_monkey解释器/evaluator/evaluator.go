package evaluator

import (
	"monkey/ast"
	"monkey/object"
)

// ============================================================
// 全局变量（常用对象复用）
// ============================================================
var (
	TRUE  = &object.Boolean{Value: true}
	FALSE = &object.Boolean{Value: false}
	NULL  = &object.Null{}
)

// Eval 是求值器的入口函数，它接收一个 AST 节点并返回一个 Object
func Eval(node ast.Node, env *object.Environment) object.Object {
	switch node := node.(type) {
	case *ast.Program:
		return evalProgram(node, env)

	case *ast.ExpressionStatement:
		return Eval(node.Expression, env)

	case *ast.IntegerLiteral:
		return &object.Integer{Value: node.Value}

	case *ast.Boolean:
		return nativeBoolToBooleanObject(node.Value)

	case *ast.PrefixExpression:
		right := Eval(node.Right, env)
		return evalPrefixExpression(node.Operator, right)

	case *ast.InfixExpression:
		left := Eval(node.Left, env)
		right := Eval(node.Right, env)
		return evalInfixExpression(node.Operator, left, right)

	case *ast.IfExpression:
		return evalIfExpression(node, env)

	case *ast.ReturnStatement:
		val := Eval(node.ReturnValue, env)
		return &object.ReturnValue{Value: val}

	case *ast.BlockStatement:
		return evalBlockStatement(node, env)

	case *ast.LetStatement:
		val := Eval(node.Value, env)
		env.Set(node.Name.Value, val)
		return nil

	case *ast.Identifier:
		return evalIdentifier(node, env)

	case *ast.FunctionLiteral:
		params := node.Parameters
		body := node.Body
		return &object.Function{Parameters: params, Body: body, Env: env}

	case *ast.CallExpression:
		function := Eval(node.Function, env)
		args := evalExpressions(node.Arguments, env)
		if len(args) == 1 && isError(args[0]) {
			return args[0]
		}

		return applyFunction(function, args)
	}

	return NULL
}

func evalProgram(program *ast.Program, env *object.Environment) object.Object {
	var result object.Object
	for _, statement := range program.StatementList {
		result = Eval(statement, env)
		returnVal, ok := result.(*object.ReturnValue)
		if ok {
			return returnVal.Value
		}
	}

	return result
}

func nativeBoolToBooleanObject(input bool) *object.Boolean {
	if input {
		return TRUE
	}

	return FALSE
}

func evalPrefixExpression(operator string, right object.Object) object.Object {
	switch operator {
	case "!":
		return evalBangOperatorExpression(right)
	case "-":
		return evalMinusPrefixOperatorExpression(right)
	default:
		return NULL
	}
}

func evalBangOperatorExpression(right object.Object) object.Object {
	switch right {
	case TRUE:
		return FALSE
	case FALSE:
		return TRUE
	case NULL:
		return TRUE
	default:
		return FALSE
	}
}

func evalMinusPrefixOperatorExpression(right object.Object) object.Object {
	if right.Type() != object.INTEGER_OBJ {
		return NULL
	}

	value := right.(*object.Integer).Value
	return &object.Integer{Value: -1 * value}
}

func evalInfixExpression(operator string, left object.Object, right object.Object) object.Object {
	// 处理整数运算
	if left.Type() == object.INTEGER_OBJ && right.Type() == object.INTEGER_OBJ {
		return evalIntegerInfixExpression(operator, left, right)
	}

	// 处于bool值运算
	if left.Type() == object.BOOLEAN_OBJ && right.Type() == object.BOOLEAN_OBJ {
		return evalBooleanInfixExpression(operator, left, right)
	}

	return NULL
}

func evalIntegerInfixExpression(operator string, left object.Object, right object.Object) object.Object {
	leftVal := left.(*object.Integer).Value
	rightVal := right.(*object.Integer).Value

	switch operator {
	case "+":
		return &object.Integer{Value: leftVal + rightVal}
	case "-":
		return &object.Integer{Value: leftVal - rightVal}
	case "*":
		return &object.Integer{Value: leftVal * rightVal}
	case "/":
		if 0 == rightVal {
			return NULL
		}

		return &object.Integer{Value: leftVal / rightVal}
	case "<":
		return nativeBoolToBooleanObject(leftVal < rightVal)
	case ">":
		return nativeBoolToBooleanObject(leftVal > rightVal)
	case "==":
		return nativeBoolToBooleanObject(leftVal == rightVal)
	case "!=":
		return nativeBoolToBooleanObject(leftVal != rightVal)
	default:
		return NULL
	}
}

func evalBooleanInfixExpression(operator string, left object.Object, right object.Object) object.Object {
	leftVal := left.(*object.Boolean).Value
	rightVal := right.(*object.Boolean).Value

	switch operator {
	case "==":
		return nativeBoolToBooleanObject(leftVal == rightVal)
	case "!=":
		return nativeBoolToBooleanObject(leftVal != rightVal)
	default:
		return NULL
	}
}

func evalIfExpression(ie *ast.IfExpression, env *object.Environment) object.Object {
	condition := Eval(ie.Condition, env)
	if isTruth(condition) {
		return Eval(ie.Consequence, env)
	} else if nil != ie.Alternative {
		return Eval(ie.Alternative, env)
	} else {
		return NULL
	}
}

func isTruth(obj object.Object) bool {
	switch obj {
	case TRUE:
		return true
	case FALSE:
		return false
	case NULL:
		return false
	default:
		return true
	}
}

func evalBlockStatement(node *ast.BlockStatement, env *object.Environment) object.Object {
	var result object.Object
	for _, tt := range node.Statements {
		result = Eval(tt, env)
		if nil != result && object.RETURN_VALUE_OBJ == result.Type() {
			return result
		}
	}

	return result
}

// evalIdentifier 查找并返回标识符的值
func evalIdentifier(node *ast.Identifier, env *object.Environment) object.Object {
	value, ok := env.Get(node.Value)
	if !ok {
		return NULL
	}

	return value
}

/*
case *ast.CallExpression:
		function := Eval(node.Function, env)
		args := evalExpressions(node.Arguments, env)
		if len(args) == 1 && isError(args[0]) {
			return agrs[0]
		}

		return applyFunction(function, args)
*/

// evalExpressions 求值参数列表
func evalExpressions(arguments []ast.Expression, env *object.Environment) []object.Object {
	var result_array []object.Object
	for _, arg := range arguments {
		result_obj := Eval(arg, env)
		if isError(result_obj) {
			return []object.Object{result_obj}
		}

		result_array = append(result_array, result_obj)
	}

	return result_array
}

func applyFunction(fn object.Object, args []object.Object) object.Object {
	switch fn := fn.(type) {
	case *object.Function:
		extendEnv := object.NewEnclosedEnvironment(fn.Env)
		for index, param := range fn.Parameters {
			extendEnv.Set(param.Value, args[index])
		}

		result := Eval(fn.Body, extendEnv)
		if returnVal, ok := result.(*object.ReturnValue); ok {
			return returnVal.Value
		}

		return result
	default:
		// 非函数对象被调用，返回错误
		return newError("not a function:%s", fn.Type())
	}
}

// isError 判断对象是否为错误对象（暂未实现错误类型，可简化）
func isError(obj object.Object) bool {
	// 先占位，后续会实现错误对象
	return false
}

// newError 创建错误对象(先简单实现，后续完善)
func newError(format string, a ...interface{}) object.Object {
	// 返回 NULL 或特殊错误对象
	return NULL
}
