package code

import (
	"bytes"
	"testing"
)

func TestMake(t *testing.T) {
	tests := []struct {
		op       Opcode
		operands []int
		expected []byte
	}{
		{OpConstant, []int{65534}, []byte{byte(OpConstant), 255, 254}},
	}

	for _, tt := range tests {
		instruction := Make(tt.op, tt.operands...)
		if len(instruction) != len(tt.expected) {
			t.Errorf("instruction length wrong: wanted=%d, got=%d", len(tt.expected), len(instruction))
		}

		for i, b := range tt.expected {
			if b != instruction[i] {
				t.Errorf("wrong byte at %d: want=%d, got=%d", i, b, instruction[i])
			}
		}
	}
}

func TestDisassemble(t *testing.T) {
	instructions := Make(OpConstant, 123)
	output := Disassmble(instructions)

	expected := "0000: OpConstant 123\n"
	if output != expected {
		t.Errorf("disassembly mismatch:\nexpected:\n%s\ngot:\n%s", expected, output)
	}
}

func TestJumpInstruction(t *testing.T) {
	ins := Make(OpJumpNotTruthy, 1234)
	expected := []byte{byte(OpJumpNotTruthy), 0x04, 0xD2}
	if !bytes.Equal(ins, expected) {
		t.Errorf("wrong instruction: expected %v, got %v", expected, ins)
	}

	ins = Make(OpJump, 5678)
	expected = []byte{byte(OpJump), 0x16, 0x2E}
	if !bytes.Equal(ins, expected) {
		t.Errorf("wrong instruction: expected %v, got %v", expected, ins)
	}
}
