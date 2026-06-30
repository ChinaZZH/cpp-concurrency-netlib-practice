package compiler

import (
	"fmt"
	"monkey/ast"
	"monkey/code"
)

type Compiler struct {
	instructions []byte
	constants    []interface{} // 常量池
}

func New() *Compiler {
	return &Compiler{
		instructions: []byte{},
		constants:    []interface{}{},
	}
}

// addConstant 向常量池添加一个常量，返回其索引
func (c *Compiler) addConstant(obj interface{}) int {
	c.constants = append(c.constants, obj)
	return len(c.constants) - 1
}

// Compile 编译 AST 节点，生成字节码
func (c *Compiler) Compile(node ast.Node) error {
	switch node := node.(type) {
	case *ast.Program:
		for _, statement := range node.StatementList {
			if err := c.Compile(statement); nil != err {
				return err
			}
		}

		return nil

	case *ast.ExpressionStatement:
		err := c.Compile(node.Expression)
		return err

	case *ast.InfixExpression:
		err := c.Compile(node.Left)
		if nil != err {
			return err
		}

		err = c.Compile(node.Right)
		if nil != err {
			return err
		}

		var instruction []byte
		switch node.Operator {
		case "+":
			instruction = code.Make(code.OpAdd)
		case "-":
			instruction = code.Make(code.OpSub)
		case "*":
			instruction = code.Make(code.OpMul)
		case "/":
			instruction = code.Make(code.OpDiv)
		case ">":
			instruction = code.Make(code.OpGreaterThan)
		case "<":
			instruction = code.Make(code.OpLessThan)
		case "==":
			instruction = code.Make(code.OpEqual)
		case "!=":
			instruction = code.Make(code.OpNotEqual)
		default:
			return fmt.Errorf("unknown operator: %s", node.Operator)
		}

		c.instructions = append(c.instructions, instruction...)
		return nil

	case *ast.IntegerLiteral:
		// 将整数常量存入常量表
		constantIndex := c.addConstant(node.Value)

		instruction := code.Make(code.OpConstant, constantIndex)
		c.instructions = append(c.instructions, instruction...)
		return nil
	}

	return fmt.Errorf("unsupported node type: %T", node)
}

// Bytecode 返回生成的字节码
func (c *Compiler) ByteCode() []byte {
	return c.instructions
}

// Constants 返回常量池（用于测试/调试）
func (c *Compiler) Constants() []interface{} {
	return c.constants
}
