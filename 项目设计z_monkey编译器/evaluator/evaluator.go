package evaluator

import (
	"fmt"
	"monkey/ast"
	"monkey/builtin"
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

	case *ast.StringLiteral:
		return &object.String{Value: node.Value}

	case *ast.ArrayLiteral:
		elems := evalExpressions(node.Elements, env)
		if len(elems) == 1 && isError(elems[0]) {
			return elems[0]
		}

		return &object.Array{Element: elems}

	case *ast.IndexExpression:
		left := Eval(node.Left, env)
		if isError(left) {
			return left
		}

		index := Eval(node.Index, env)
		if isError(left) {
			return index
		}

		return evalIndexExpression(left, index)

	case *ast.HashLiteral:
		return evalHashLiteral(node, env)
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

	// 处理字符串运算
	if left.Type() == object.STRING_OBJ && right.Type() == object.STRING_OBJ {
		return evalStringObjInfixExpression(operator, left, right)
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

func evalStringObjInfixExpression(operator string, left object.Object, right object.Object) object.Object {
	if "+" != operator {
		return newError("unknown operator: %s %s %s", left.Type(), operator, right.Type())
	}

	resultVal := left.(*object.String).Value
	resultVal += right.(*object.String).Value
	return &object.String{Value: resultVal}
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
	if ok {
		return value
	}

	builtinObj, ok := builtin.LookUp(node.Value)
	if ok {
		return builtinObj
	}

	return NULL
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
	case *object.Builtin:
		return fn.Fn(args...)
	default:
		// 非函数对象被调用，返回错误
		return newError("not a function:%s", fn.Type())
	}
}

func evalIndexExpression(left object.Object, index object.Object) object.Object {
	switch {
	case object.ARRAY_OBJ == left.Type() && object.INTEGER_OBJ == index.Type():
		return evalArrayIndexExpression(left, index)
	case object.HASH_OBJ == left.Type():
		return evalHashIndexExpression(left, index)
	default:
		return newError("index operator not supported: %s", left.Type())
	}
}

func evalArrayIndexExpression(arrayObj object.Object, indexObj object.Object) object.Object {
	array := arrayObj.(*object.Array)
	index := indexObj.(*object.Integer).Value
	max := int64(len(array.Element) - 1)
	if index < 0 || index > max {
		return NULL
	}

	return array.Element[index]
}

func evalHashIndexExpression(hashObj object.Object, indexObj object.Object) object.Object {
	hash := hashObj.(*object.Hash)

	hasher, ok := indexObj.(object.Hasher)
	if !ok {
		return newError("unusable as hash key: %s", indexObj.Type())
	}

	// 没找到
	hashPair, ok := hash.Pairs[hasher.HashKey()]
	if !ok {
		return NULL
	}

	return hashPair.Value
}

func evalHashLiteral(node *ast.HashLiteral, env *object.Environment) object.Object {
	/*
		type HashLiteral struct {
			Token token.Token
			Pairs map[Expression]Expression
		}
	*/

	pairs := make(map[object.HashKey]object.HashPair)
	for keyExpssion, valExpression := range node.Pairs {
		keyObj := Eval(keyExpssion, env)
		if isError(keyObj) {
			return keyObj
		}

		haseher, ok := keyObj.(object.Hasher)
		if !ok {
			return newError("unusable as hash key: %s", keyObj.Type())
		}

		valueObj := Eval(valExpression, env)
		if isError(keyObj) {
			return keyObj
		}

		pairs[haseher.HashKey()] = object.HashPair{Key: keyObj, Value: valueObj}
	}

	return &object.Hash{Pairs: pairs}
}

// isError 判断对象是否为错误对象（暂未实现错误类型，可简化）
func isError(obj object.Object) bool {
	// 先占位，后续会实现错误对象
	return false
}

// newError 创建错误对象(先简单实现，后续完善)
func newError(format string, a ...interface{}) object.Object {
	// 返回 NULL 或特殊错误对象
	return &object.Error{Message: fmt.Sprintf(format, a...)}
}
