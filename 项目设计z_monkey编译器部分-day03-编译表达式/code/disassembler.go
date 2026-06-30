package code

import (
	"fmt"
	"strings"
)

// Disassemble 将字节码指令序列转换为可读字符串
func Disassmble(instructions []byte) string {
	var out strings.Builder
	i := 0
	for i < len(instructions) {
		op := Opcode(instructions[i])
		def, ok := Lookup(op)
		if !ok {
			out.WriteString(fmt.Sprintf("ERROR: unknow opcode %d\n", op))
			i++
			continue
		}

		// 记录操作码的index
		code_operand := i
		// 下面开始操作数的
		i += 1
		operands := make([]int, len(def.OperandWidths))
		total_operator_bytes := 0
		for j, width := range def.OperandWidths {
			startIdx := i + total_operator_bytes
			if startIdx >= len(instructions) {
				out.WriteString(fmt.Sprintf("ERROR: incomplete instruction at %d\n", i))
				break
			}

			switch width {
			case 1:
				operands[j] = int(instructions[startIdx])
			case 2:
				operands[j] = (int(instructions[startIdx]) << 8) + int(instructions[startIdx+1])
			}

			total_operator_bytes += width
		}

		// 调到下一个操作码上
		i += total_operator_bytes

		// 格式化输出
		out.WriteString(fmt.Sprintf("%04d: %s", code_operand, def.Name))
		for _, num := range operands {
			out.WriteString(fmt.Sprintf(" %d", num))
		}

		out.WriteString("\n")
	}

	return out.String()
}
