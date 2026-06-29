package compiler

import "monkey/ast"

type Compiler struct {
	instructions []byte
	constants    []interface{}
}

func New() *Compiler {
	return &Compiler{
		instructions: []byte{},
		constants:    []interface{}{},
	}
}

func (c *Compiler) Compiler(node ast.Node) error {
	return nil
}

func (c *Compiler) ByteCode() []byte {
	return c.instructions
}
