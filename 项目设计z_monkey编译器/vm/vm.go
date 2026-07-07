package vm

import (
	"fmt"
	"monkey/code"
	"monkey/object"
	"sync"
)

// GlobalsSize 定义全局变量最大数量

const GlobalsSize = 65536

// Frame 表示一个函数调用帧
type Frame struct {
	fn     *object.Closure // 指向被调用的闭包
	ip     int             // 指令指针（当前执行位置）
	locals []object.Object //局部变量
}

type VM struct {
	globals   []object.Object // 全局变量存储
	frames    []*Frame        // 调用帧栈
	frameIdex int             // 当前帧索引
	stack     []object.Object // 操作数栈
	sp        int             // 栈指针
	framePool sync.Pool       // 内存池
}

func New(bytecode []byte, constants []interface{}) *VM {
	mainClosure := &object.Closure{
		Fn: &object.CompiledFunction{
			Instructions: bytecode,
			Constants:    constants,
			NumLocals:    0,
			NumParams:    0,
		},
		Free: []object.Object{},
	}

	// 创建虚拟机并且将主函数变成主闭包函数放入函数调用栈中
	vm := &VM{
		globals:   make([]object.Object, GlobalsSize),
		frames:    []*Frame{},
		frameIdex: 0,
		stack:     make([]object.Object, 0, 2048), // 预分配栈空间
		sp:        0,
		framePool: sync.Pool{New: func() interface{} { return &Frame{locals: make([]object.Object, 0, 8)} }},
	}

	mainParam := []object.Object{}
	vm.newFrame(mainClosure, 0, mainParam)
	return vm
}

// 数据栈操作辅助方法 入栈和出栈
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

func (vm *VM) showStackInfo() {
	for index, numObject := range vm.stack {
		switch numObject := numObject.(type) {
		case (*object.CompiledFunction):
			fmt.Printf("index: %d  Function (params=%d, locals=%d, free=%d numObject=%p):\n", index, numObject.NumParams, numObject.NumLocals, numObject.NumFree, &numObject)
		case (*object.Boolean):
			fmt.Printf("index: %d Boolean value:=%t\n", index, numObject.Value)
		case (*object.Integer):
			fmt.Printf("index: %d Integer value:=%d\n", index, numObject.Value)
		case (*object.String):
			fmt.Printf("index: %d String value:=%s\n", index, numObject.Value)
		default:
			fmt.Printf("index: %d default value:=%t\n", index, numObject)
		}
	}
}

// 获取数据栈的top元素
func (vm *VM) LastPoppedStackElem() object.Object {
	if vm.sp > 0 {
		return vm.stack[vm.sp-1]
	}

	return nil
}

// Run 执行字节码
func (vm *VM) Run() error {

	// 取出当前函数帧的指令集，按照指令集取出操作数和操作码，按照顺序执行
	for {
		frame := vm.currentFrame()
		if frame.ip >= len(frame.fn.Fn.Instructions) {
			break
		}

		// 取出当前的指令操作码
		frame_instructions := frame.fn.Fn.Instructions
		op := code.Opcode(frame_instructions[frame.ip])
		// 转移到下一条字节码(操作数或者下一条指令)
		frame.ip += 1

		switch op {
		// ========== 常量与字面量 ==========
		case code.OpConstant:
			// 根据操作数计算出常量在当前函数常量池中的索引位置，并且将指令地址往后移2位
			idx := int(frame_instructions[frame.ip])<<8 | int(frame_instructions[frame.ip+1])
			frame.ip += 2

			var obj object.Object
			raw := frame.fn.Fn.Constants[idx]

			// 取出常量并且转化为object放入操作数栈
			switch v := raw.(type) {
			case int64:
				obj = &object.Integer{Value: v}
			case int:
				obj = &object.Integer{Value: int64(v)}
			case string:
				obj = &object.String{Value: v}
			case bool:
				obj = &object.Boolean{Value: v}
			case *object.Builtin:
				obj = v
			default:
				return fmt.Errorf("unsupported constant type: %T", raw)
			}

			vm.push(obj)

		// code.OpTrue code.OpFalse code.OpNull 将操作数推入到操作数栈中
		case code.OpTrue:
			vm.push(True)
		case code.OpFalse:
			vm.push(False)
		case code.OpNull:
			vm.push(Null)
		case code.OpArray:
			// 根据操作数计算出数组长度，并且将指令地址往后移2位
			array_len := int(frame_instructions[frame.ip])<<8 | int(frame_instructions[frame.ip+1])
			frame.ip += 2

			// 根据操作数栈弹出对应的数组元素，并且按照栈的顺序还原。然后将ArrayObject放入操作数栈中
			array_obj := &object.Array{Element: make([]object.Object, array_len)}
			for i := array_len - 1; i >= 0; i-- {
				array_obj.Element[i] = vm.pop()
			}

			vm.push(array_obj)
		case code.OpHash:
			pair_len := int(frame_instructions[frame.ip])<<8 | int(frame_instructions[frame.ip+1])
			frame.ip += 2

			pairs := make(map[object.HashKey]object.HashPair)
			for i := 0; i < pair_len; i++ {
				value := vm.pop()
				key := vm.pop()

				hasher, ok := key.(object.Hasher)
				if !ok {
					return fmt.Errorf("key object can not hasher: got_index = %T", key)
				}

				pairs[hasher.HashKey()] = object.HashPair{Key: key, Value: value}
			}

			vm.push(&object.Hash{Pairs: pairs})
		case code.OpIndex:
			index := vm.pop()
			leftObj := vm.pop()

			switch leftObj := leftObj.(type) {
			case *object.Array:
				integer_idx, ok := index.(*object.Integer)
				if !ok {

				}

				idx := integer_idx.Value
				if idx < 0 || idx >= int64(len(leftObj.Element)) {
					return fmt.Errorf("index out of bounds: got_index = %d max_len = %d", idx, len(leftObj.Element))
				}

				vm.push(leftObj.Element[idx])
			case *object.Hash:
				hasher, ok := index.(object.Hasher)
				if !ok {
					return fmt.Errorf("code.OpIndex can not hasher: got_index = %T", index)
				}

				hashKey := hasher.HashKey()
				hashVal, ok := leftObj.Pairs[hashKey]
				if !ok {
					vm.push(Null)
				} else {
					vm.push(hashVal.Value)
				}
			default:
				return fmt.Errorf("index operator not supported on %s", leftObj.Type())
			}

		// ========== 算术运算 ==========
		case code.OpAdd:
			if len(vm.stack) < 2 {
				return fmt.Errorf("stack length must be >= 2 got=%d", len(vm.stack))
			}

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
			// 取出跳转的相对偏移地址offset
			offset := int(frame_instructions[frame.ip])<<8 | int(frame_instructions[frame.ip+1])
			frame.ip += 2

			// 当条件为假的时候，指令寄存器的地址偏移位置+offset.即frame.ip += offset
			condition := vm.pop()
			if !isTruthy(condition) {
				frame.ip += offset
			}

		case code.OpJump:
			offset := int(frame_instructions[frame.ip])<<8 | int(frame_instructions[frame.ip+1])
			frame.ip += 2
			frame.ip += offset

			// ========== 全局变量 ==========
		case code.OpSetGlobal:
			idx := int(frame_instructions[frame.ip])<<8 | int(frame_instructions[frame.ip+1])
			frame.ip += 2

			global_data := vm.pop()
			vm.globals[idx] = global_data
		case code.OpGetGlobal:
			idx := int(frame_instructions[frame.ip])<<8 | int(frame_instructions[frame.ip+1])
			frame.ip += 2

			val := vm.globals[idx]
			if val == nil {
				return fmt.Errorf("global variable not set at index %d", idx)
			}

			vm.push(val)

			// ========== 本地局部变量 ==========
		case code.OpSetLocal:
			idx := int(frame_instructions[frame.ip])<<8 | int(frame_instructions[frame.ip+1])
			frame.ip += 2

			local_data := vm.pop()
			frame.locals[idx] = local_data
		case code.OpGetLocal:
			idx := int(frame_instructions[frame.ip])<<8 | int(frame_instructions[frame.ip+1])
			frame.ip += 2

			val := frame.locals[idx]
			if val == nil {
				return fmt.Errorf("global variable not set at index %d", idx)
			}

			vm.push(val)
		// ========== 自由变量变量 ==========
		case code.OpGetFree:
			idx := int(frame_instructions[frame.ip])<<8 | int(frame_instructions[frame.ip+1])
			frame.ip += 2

			if idx < 0 || idx >= len(frame.fn.Free) {
				return fmt.Errorf("free variable not set at index %d", idx)
			}

			vm.push(frame.fn.Free[idx])
			// ========== 函数相关 ==========
		case code.OpClosure:
			// 获取常量的索引值和对应的自由变量的个数
			idx := int(frame_instructions[frame.ip])<<8 | int(frame_instructions[frame.ip+1])
			numFree := int(frame_instructions[frame.ip+2])
			frame.ip += 3

			compiled_fun, ok := frame.fn.Fn.Constants[idx].(*object.CompiledFunction)
			if !ok {
				return fmt.Errorf("code.OpClosure get function error, got %t", frame.fn.Fn.Constants[idx])
			}

			// 通过操作数栈stack弹出对应的自由变量
			free := make([]object.Object, numFree)
			for i := numFree - 1; i >= 0; i-- {
				free[i] = vm.pop()
			}

			closure := &object.Closure{
				Fn:   compiled_fun,
				Free: free,
			}

			vm.push(closure)

		case code.OpCall:
			// 读取参数个数
			param_num := int(frame_instructions[frame.ip])
			frame.ip += 1

			// 从栈顶弹出参数（顺序：先压入的参数在栈底，最后一个参数在栈顶）
			// 我们按顺序弹出到切片中（但我们需要按参数顺序绑定）
			// 由于栈是后进先出，我们提前分配切片，从栈中按顺序提取
			params := make([]object.Object, param_num)
			for i := param_num - 1; i >= 0; i-- {
				params[i] = vm.pop()
			}

			// 获取被调用的闭包（在栈顶）
			funcObj := vm.pop()
			switch funcObj := funcObj.(type) {
			case *object.Closure:
				// 创建新帧
				// 注意：NumLocals 包含参数和局部变量，目前我们只设置参数，局部变量后续实现
				// 为了简单，我们假设 NumLocals 至少等于参数个数，并将参数放入 locals
				vm.newFrame(funcObj, funcObj.Fn.NumLocals, params)
			case *object.Builtin:
				// 调用内置函数
				reusltObj := funcObj.Fn(params...)
				vm.push(reusltObj)
			default:
				return fmt.Errorf("code.Closure get type error, got %t", funcObj)
			}
		// 会弹出一个object.value返回值对象，然后压入一个objetc.value返回值对象，相互抵消则不进行入栈出栈的操作
		case code.OpReturnVal:
			vm.popFrame()
		// 会弹出一个null对象，然后压入一个null对象，相互抵消则不进行入栈出栈的操作
		case code.OpReturn:
			vm.popFrame()
		default:
			return fmt.Errorf("unknown opcode: %d", op)

		}
	}

	return nil
}

// 返回当前正在执行的函数帧 栈帧通过vm.frameIdex表示当前所正在使用的函数帧
func (vm *VM) currentFrame() *Frame {
	return vm.frames[vm.frameIdex]
}

// 当开始调用新的函数的时候就会创建新的函数帧并且将其放入到函数栈中。
// 使用sync.Pool对象池技术进行管理
func (vm *VM) newFrame(closure *object.Closure, numlocal int, closureParam []object.Object) {
	frame := vm.framePool.Get().(*Frame)
	frame.fn = closure
	frame.ip = 0

	if cap(frame.locals) < numlocal {
		frame.locals = make([]object.Object, 0, numlocal)
	}

	frame.locals = frame.locals[:numlocal]
	for i := 0; i < numlocal; i++ {
		frame.locals[i] = nil
	}

	copy(frame.locals, closureParam)
	vm.frames = append(vm.frames, frame)
	vm.frameIdex = len(vm.frames) - 1
}

// 函数调用结束，则将函数从函数栈中出栈
// 使用sync.Pool对象池技术进行管理
func (vm *VM) popFrame() {
	current_frame := vm.currentFrame()
	for i := range current_frame.locals {
		current_frame.locals[i] = nil
	}
	current_frame.locals = current_frame.locals[:0]
	vm.framePool.Put(current_frame)

	vm.frames = vm.frames[:vm.frameIdex]
	vm.frameIdex -= 1
}

// ========== 辅助函数 ==========

// 预定义的布尔对象
var (
	True  = &object.Boolean{Value: true}
	False = &object.Boolean{Value: false}
)

// bool为true返回True对象，否则返回false对象
func nativeBoolToBooleanObject(b bool) *object.Boolean {
	if b {
		return True
	}
	return False
}

// 判断是非的逻辑
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

// 根据object来进行取反
func bangOperatorToBooleanObject(obj object.Object) *object.Boolean {
	if isTruthy(obj) {
		return False
	}

	return True
}
