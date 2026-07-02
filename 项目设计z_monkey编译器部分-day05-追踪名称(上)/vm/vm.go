package vm

import (
	"fmt"
	"monkey/code"
	"monkey/object"
)

// GlobalsSize 定义全局变量最大数量

const GlobalsSize = 65536

type VM struct {
	instructions []byte          // 字节码指令序列
	constants    []interface{}   // 常量池
	globals      []object.Object // 全局变量存储
	stack        []object.Object // 操作数栈
	sp           int             // 栈指针
}

func New(bytecode []byte, constants []interface{}) *VM {
	return &VM{
		instructions: bytecode, // 存储字节码
		constants:    constants,
		globals:      make([]object.Object, GlobalsSize),
		stack:        make([]object.Object, 0, 2048), // 预分配栈空间
		//stack: []object.Object{}, // 预分配栈空间
		sp: 0,
	}
}

// 栈操作辅助方法
func (vm *VM) push(obj object.Object) {
	vm.stack = append(vm.stack, obj)
	vm.sp++
}

func (vm *VM) pop() object.Object {
	if 0 == vm.sp {
		return nil
	}

	vm.sp--
	obj := vm.stack[vm.sp]
	vm.stack = vm.stack[:vm.sp]
	return obj
}

func (vm *VM) LastPoppedStackElem() object.Object {
	if vm.sp > 0 {
		return vm.stack[vm.sp-1]
	}

	return nil
}

// Run 执行字节码
func (vm *VM) Run() error {
	//fmt.Println("vm run")

	for ip := 0; ip < len(vm.instructions); {
		op := code.Opcode(vm.instructions[ip])
		ip += 1 // 转移到下一条字节码(操作数或者下一条指令)
		//fmt.Printf("vm opCode :%d\n", op)

		switch op {
		// ========== 常量与字面量 ==========
		case code.OpConstant:
			//fmt.Printf("vm.instructions lenght :%d, ip%d\n", len(vm.instructions), ip)
			idx := int(vm.instructions[ip])<<8 | int(vm.instructions[ip+1])
			ip += 2

			//fmt.Printf("vm.constants totallength :%d\n", len(vm.constants))
			var obj object.Object
			raw := vm.constants[idx]
			//fmt.Printf("vm.constants index :%d\n", idx)

			//fmt.Printf("OpConstant type %t", raw)
			switch v := raw.(type) {
			case int64:
				obj = &object.Integer{Value: v}
			case int:
				obj = &object.Integer{Value: int64(v)}
			case string:
				obj = &object.String{Value: v}
			case bool:
				obj = &object.Boolean{Value: v}
			default:
				return fmt.Errorf("unsupported constant type: %T", raw)
			}

			vm.push(obj)
		case code.OpTrue:
			vm.push(True)
		case code.OpFalse:
			vm.push(False)
		case code.OpNull:
			vm.push(Null)

		// ========== 算术运算 ==========
		case code.OpAdd:
			right := vm.pop()
			left := vm.pop()

			// 目前支持整数加法和字符串加法，遇到其他类型返回错误
			if object.INTEGER_OBJ == left.Type() && object.INTEGER_OBJ == right.Type() {
				sum := left.(*object.Integer).Value + right.(*object.Integer).Value
				vm.push(&object.Integer{Value: sum})
			} else if object.STRING_OBJ == left.Type() && object.STRING_OBJ == right.Type() {
				new_string := left.(*object.String).Value + right.(*object.String).Value
				vm.push(&object.String{Value: new_string})
			} else {
				return fmt.Errorf("unsupported types for addition: %s + %s", left.Type(), right.Type())
			}

		case code.OpSub:
			right := vm.pop()
			left := vm.pop()

			// 目前仅支持整数减法，遇到其他类型返回错误
			if object.INTEGER_OBJ != left.Type() || object.INTEGER_OBJ != right.Type() {
				return fmt.Errorf("unsupported types for subtraction: %s - %s", left.Type(), right.Type())
			}

			sum := left.(*object.Integer).Value - right.(*object.Integer).Value
			vm.push(&object.Integer{Value: sum})

		case code.OpMul:
			right := vm.pop()
			left := vm.pop()

			// 目前仅支持整数乘法，遇到其他类型返回错误
			if object.INTEGER_OBJ != left.Type() || object.INTEGER_OBJ != right.Type() {
				return fmt.Errorf("unsupported types for multiplication: %s + %s", left.Type(), right.Type())
			}

			sum := left.(*object.Integer).Value * right.(*object.Integer).Value
			vm.push(&object.Integer{Value: sum})
		case code.OpDiv:
			right := vm.pop()
			left := vm.pop()

			// 目前仅支持整数除法，遇到其他类型返回错误
			if object.INTEGER_OBJ != left.Type() || object.INTEGER_OBJ != right.Type() {
				return fmt.Errorf("unsupported types for division: %s / %s", left.Type(), right.Type())
			}

			sum := left.(*object.Integer).Value / right.(*object.Integer).Value
			vm.push(&object.Integer{Value: sum})

			// ========== 比较运算 ==========
		case code.OpGreaterThan:
			right := vm.pop()
			left := vm.pop()

			// 目前仅支持整数的>比较，遇到其他类型返回错误
			if object.INTEGER_OBJ != left.Type() || object.INTEGER_OBJ != right.Type() {
				return fmt.Errorf("unsupported types for greater than: %s > %s", left.Type(), right.Type())
			}

			leftVal := left.(*object.Integer).Value
			rightVal := right.(*object.Integer).Value
			result_obj := nativeBoolToBooleanObject(leftVal > rightVal)
			vm.push(result_obj)
		case code.OpLessThan:
			right := vm.pop()
			left := vm.pop()

			// 目前仅支持整数的<比较，遇到其他类型返回错误
			if object.INTEGER_OBJ != left.Type() || object.INTEGER_OBJ != right.Type() {
				return fmt.Errorf("unsupported types for less than: %s < %s", left.Type(), right.Type())
			}

			leftVal := left.(*object.Integer).Value
			rightVal := right.(*object.Integer).Value
			result_obj := nativeBoolToBooleanObject(leftVal < rightVal)
			vm.push(result_obj)

		case code.OpEqual:
			right := vm.pop()
			left := vm.pop()

			// 目前仅支持整数和bool值比较，遇到其他类型返回错误
			if object.INTEGER_OBJ == left.Type() && object.INTEGER_OBJ == right.Type() {
				leftVal := left.(*object.Integer).Value
				rightVal := right.(*object.Integer).Value
				result_obj := nativeBoolToBooleanObject(leftVal == rightVal)
				vm.push(result_obj)
			} else if object.BOOLEAN_OBJ == left.Type() && object.BOOLEAN_OBJ == right.Type() {
				leftVal := left.(*object.Boolean).Value
				rightVal := right.(*object.Boolean).Value
				result_obj := nativeBoolToBooleanObject(leftVal == rightVal)
				vm.push(result_obj)
			} else {
				return fmt.Errorf("unsupported types for equal: %s == %s", left.Type(), right.Type())
			}

		case code.OpNotEqual:
			right := vm.pop()
			left := vm.pop()

			// 目前仅支持整数和bool值比较，遇到其他类型返回错误
			if object.INTEGER_OBJ == left.Type() && object.INTEGER_OBJ == right.Type() {
				leftVal := left.(*object.Integer).Value
				rightVal := right.(*object.Integer).Value
				result_obj := nativeBoolToBooleanObject(leftVal != rightVal)
				vm.push(result_obj)
			} else if object.BOOLEAN_OBJ == left.Type() && object.BOOLEAN_OBJ == right.Type() {
				leftVal := left.(*object.Boolean).Value
				rightVal := right.(*object.Boolean).Value
				result_obj := nativeBoolToBooleanObject(leftVal != rightVal)
				vm.push(result_obj)
			} else {
				return fmt.Errorf("unsupported types for not equal: %s != %s", left.Type(), right.Type())
			}
			// ========== 前缀运算符 ==========
		case code.OpBang:
			operand := vm.pop()
			/*
				truthy := isTruthy(operand)
				if truthy {
					vm.push(False)
				} else {
					vm.push(True)
				}
			*/

			result_obj := bangOperatorToBooleanObject(operand)
			vm.push(result_obj)

		case code.OpMinus:
			operand := vm.pop()
			if object.INTEGER_OBJ != operand.Type() {
				return fmt.Errorf("unsupported types for OpMinus: %s ", operand.Type())
			}

			result_value := operand.(*object.Integer).Value
			result_value *= -1
			vm.push(&object.Integer{Value: result_value})

			// ========== 条件跳转 ==========
		case code.OpJumpNotTruthy:
			offset := int(vm.instructions[ip])<<8 | int(vm.instructions[ip+1])
			ip += 2

			condition := vm.pop()
			if !isTruthy(condition) {
				ip += offset
			}

		case code.OpJump:
			offset := int(vm.instructions[ip])<<8 | int(vm.instructions[ip+1])
			ip += 2
			ip += offset

			// ========== 全局变量 ==========
		case code.OpSetGlobal:
			idx := int(vm.instructions[ip])<<8 | int(vm.instructions[ip+1])
			ip += 2

			global_data := vm.pop()
			vm.globals[idx] = global_data
		case code.OpGetGlobal:
			idx := int(vm.instructions[ip])<<8 | int(vm.instructions[ip+1])
			ip += 2

			val := vm.globals[idx]
			if val == nil {
				return fmt.Errorf("global variable not set at index %d", idx)
			}

			vm.push(val)

		default:
			return fmt.Errorf("unknown opcode: %d", op)

		}
	}

	return nil
}

// ========== 辅助函数 ==========

// 预定义的布尔对象
var (
	True  = &object.Boolean{Value: true}
	False = &object.Boolean{Value: false}
)

func nativeBoolToBooleanObject(b bool) *object.Boolean {
	if b {
		return True
	}
	return False
}

func isTruthy(obj object.Object) bool {
	switch obj := obj.(type) {
	case *object.Null:
		return false
	case *object.Boolean:
		return obj.Value
	default:
		return true
	}
}

// Null 对象（需要定义，如果 object 包中没有的话）
// 如果 object.Null 已存在，可以直接引用，这里为了独立定义
var Null = &object.Null{}

func bangOperatorToBooleanObject(obj object.Object) *object.Boolean {
	if isTruthy(obj) {
		return False
	}

	return True
}
