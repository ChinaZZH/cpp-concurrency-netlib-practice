package code

import (
	"encoding/binary"
	"fmt"
)

// opcode 字节码指令类型
type Opcode byte

const (
	OpConstant Opcode = iota // 加载常量到栈
	OpAdd
	OpSub
	OpMul
	OpDiv
	OpGreaterThan
	OpLessThan
	OpEqual
	OpNotEqual
	OpTrue
	OpFalse
	OpBang  // !
	OpMinus // -（前缀）
	OpJumpNotTruthy
	OpJump
	OpSetGlobal
	OpGetGlobal
	OpNull
)

// Definition 指令定义：名称 + 操作数数量
type Definition struct {
	Name          string
	OperandWidths []int // 每个操作数的字节宽度
}

var definitions = map[Opcode]*Definition{
	OpConstant:      {"OpConstant", []int{2}},
	OpAdd:           {"OpAdd", []int{}},
	OpSub:           {"OpSub", []int{}},
	OpMul:           {"OpMul", []int{}},
	OpDiv:           {"OpDiv", []int{}},
	OpGreaterThan:   {"OpGreaterThan", []int{}},
	OpLessThan:      {"OpLessThan", []int{}},
	OpEqual:         {"OpEqual", []int{}},
	OpNotEqual:      {"OpNotEqual", []int{}},
	OpTrue:          {"OpTrue", []int{}},
	OpFalse:         {"OpFalse", []int{}},
	OpBang:          {"OpBang", []int{}},
	OpMinus:         {"OpMinus", []int{}},
	OpJumpNotTruthy: {"OpJumpNotTruthy", []int{2}},
	OpJump:          {"OpJump", []int{2}},
	OpSetGlobal:     {"OpSetGlobal", []int{2}},
	OpGetGlobal:     {"OpGetGlobal", []int{2}},
	OpNull:          {"OpNull", []int{}},
}

func Lookup(op Opcode) (*Definition, bool) {
	def, ok := definitions[op]
	return def, ok
}

// 将“抽象的操作码 + 操作数”转换为“符合指令格式的二进制字节序列”。
// Make(op, operands...) = 按照 op 的定义，把操作数和操作码打包成一个字节数组，使其符合内存中的指令格式。
func Make(op Opcode, operands ...int) []byte {
	def, ok := definitions[op]
	if !ok {
		return []byte{}
	}

	// 检查操作数数量是否匹配
	if len(operands) != len(def.OperandWidths) {
		// 可以选择 panic 或返回错误
		// 这里使用 panic 便于调试，正式环境可改为返回 error
		panic(fmt.Sprintf("operand count mismatch for %s: expected %d, got %d",
			def.Name, len(def.OperandWidths), len(operands)))

		/*
			return nil, fmt.Errorf("operand count mismatch for %s: expected %d, got %d",
				def.Name, len(def.OperandWidths), len(operands))
		*/
	}

	instructionLen := 1
	for _, w := range def.OperandWidths {
		instructionLen += w
	}

	instruction := make([]byte, instructionLen)
	instruction[0] = byte(op)

	offset := 1
	for index, o := range operands {
		width := def.OperandWidths[index]
		switch width {
		case 1:
			if o > 256 {
				panic(fmt.Sprintf("operand value %d exceeds 1-byte range for %s", o, def.Name))
			}

			instruction[offset] = byte(o)
		case 2:
			if o > 65535 {
				panic(fmt.Sprintf("operand value %d exceeds 2-byte range for %s", o, def.Name))
			}

			// 使用大端编码
			binary.BigEndian.PutUint16(instruction[offset:], uint16(o))
		default:
			panic(fmt.Sprintf("unsupported operand width: %d for %s", width, def.Name))
		}

		offset += width
	}

	return instruction
}
