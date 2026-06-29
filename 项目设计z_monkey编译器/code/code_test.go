package code

import "testing"

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
