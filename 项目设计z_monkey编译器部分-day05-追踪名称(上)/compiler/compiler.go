package compiler

import (
	"fmt"
	"monkey/ast"
	"monkey/code"
)

type Compiler struct {
	instructions []byte
	constants    []interface{} // 常量池
	symbolTable  *SymbolTable
}

func New() *Compiler {
	return &Compiler{
		instructions: []byte{},
		constants:    []interface{}{},
		symbolTable:  NewSymbolTable(),
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
			return fmt.Errorf("unknown infixExpression operator: %s", node.Operator)
		}

		c.instructions = append(c.instructions, instruction...)
		return nil

	case *ast.IntegerLiteral:
		// 将整数常量存入常量表
		constantIndex := c.addConstant(node.Value)

		instruction := code.Make(code.OpConstant, constantIndex)
		c.instructions = append(c.instructions, instruction...)
		return nil

	case *ast.Boolean:
		var instructions []byte
		if node.Value {
			instructions = code.Make(code.OpTrue)
		} else {
			instructions = code.Make(code.OpFalse)
		}

		c.instructions = append(c.instructions, instructions...)
		return nil

	case *ast.PrefixExpression:
		err := c.Compile(node.Right)
		if nil != err {
			return err
		}

		var instruction []byte
		switch node.Operator {
		case "!":
			instruction = code.Make(code.OpBang)
		case "-":
			instruction = code.Make(code.OpMinus)
		default:
			return fmt.Errorf("unknown prefix operator: %s", node.Operator)
		}

		c.instructions = append(c.instructions, instruction...)
		return nil
	case *ast.IfExpression:
		// 编译条件
		err := c.Compile(node.Condition)
		if nil != err {
			return err
		}

		jumpNotTruthPos := len(c.instructions)
		c.instructions = append(c.instructions, code.Make(code.OpJumpNotTruthy, 0xFFFF)...)

		// 编译真分支
		err = c.Compile(node.Consequence)
		if nil != err {
			return err
		}

		jumpPos := len(c.instructions)
		c.instructions = append(c.instructions, code.Make(code.OpJump, 0xFFFF)...)

		afterConsequencePos := len(c.instructions)
		offset := afterConsequencePos - (jumpNotTruthPos + 3)
		c.replaceJumpOperand(jumpNotTruthPos, offset)

		// 编译假分支
		if nil != node.Alternative {
			err = c.Compile(node.Alternative)
			if nil != err {
				return err
			}
		} else {
			c.instructions = append(c.instructions, code.Make(code.OpNull)...)
		}

		afterExpressionPos := len(c.instructions)
		offset = afterExpressionPos - (jumpPos + 3)
		c.replaceJumpOperand(jumpPos, offset)

		return nil

	case *ast.BlockStatement:
		for _, statement := range node.Statements {
			err := c.Compile(statement)
			if nil != err {
				return err
			}
		}

		return nil

	case *ast.LetStatement:
		// 1. 编译右边的表达式（值），结果会留在栈顶
		err := c.Compile(node.Value)
		if nil != err {
			return err
		}

		// 2. 在符号表中定义变量，获得索引
		symbol := c.symbolTable.Define(node.Name.Value)

		// 3. 根据作用域生成对应的指令
		if symbol.Scope == GlobalScope {
			c.instructions = append(c.instructions, code.Make(code.OpSetGlobal, symbol.Index)...)
		} else {
			// 局部变量（后续实现，先占位）
			return fmt.Errorf("local variables not implemented yet")
		}

		return nil

	case *ast.Identifier:
		// 1. 在符号表中查找变量
		sym, ok := c.symbolTable.Resolve(node.Value)
		if !ok {
			return fmt.Errorf("undefined variable: %s", node.Value)
		}

		// 根据作用域生成对应的指令
		if sym.Scope == GlobalScope {
			c.instructions = append(c.instructions, code.Make(code.OpGetGlobal, sym.Index)...)
		} else {
			// 局部变量（后续实现，先占位）
			return fmt.Errorf("local variables not implemented yet")
		}

		return nil
	}

	return fmt.Errorf("unsupported node type: %T", node)
}

func (c *Compiler) replaceJumpOperand(pos int, offset int) {
	c.instructions[pos+1] = byte(offset >> 8)
	c.instructions[pos+2] = byte(offset & 0xFF)
}

// Bytecode 返回生成的字节码
func (c *Compiler) ByteCode() []byte {
	return c.instructions
}

// Constants 返回常量池（用于测试/调试）
func (c *Compiler) Constants() []interface{} {
	return c.constants
}
