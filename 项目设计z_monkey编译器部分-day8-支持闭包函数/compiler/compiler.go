package compiler

import (
	"fmt"
	"monkey/ast"
	"monkey/builtin"
	"monkey/code"
	"monkey/object"
)

type Compiler struct {
	instructions []byte
	constants    []interface{} // 常量池
	symbolTable  *SymbolTable
	depthTest    int // 测试使用
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

	case *ast.StringLiteral:
		// 将整数常量存入常量表
		constantIndex := c.addConstant(node.Value)

		instruction := code.Make(code.OpConstant, constantIndex)
		c.instructions = append(c.instructions, instruction...)
		return nil

	case *ast.ArrayLiteral:
		for _, expression := range node.Elements {
			err := c.Compile(expression)
			if err != nil {
				return err
			}
		}

		array_len := len(node.Elements)
		instruction := code.Make(code.OpArray, array_len)
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

	case *ast.HashLiteral:
		for keyPair, valuePair := range node.Pairs {
			err := c.Compile(keyPair)
			if err != nil {
				return err
			}

			err = c.Compile(valuePair)
			if err != nil {
				return err
			}
		}

		hash_len := len(node.Pairs)
		instruction := code.Make(code.OpHash, hash_len)
		c.instructions = append(c.instructions, instruction...)
		return nil

	case *ast.IndexExpression:
		err := c.Compile(node.Left)
		if err != nil {
			return err
		}

		err = c.Compile(node.Index)
		if err != nil {
			return err
		}

		instruction := code.Make(code.OpIndex)
		c.instructions = append(c.instructions, instruction...)
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
		// 1. 在符号表中定义变量，获得索引
		symbol := c.symbolTable.Define(node.Name.Value)

		// 2. 编译右边的表达式（值），结果会留在栈顶
		err := c.Compile(node.Value)
		if nil != err {
			return err
		}

		// 3. 根据作用域生成对应的指令
		switch symbol.Scope {
		case GlobalScope:
			c.instructions = append(c.instructions, code.Make(code.OpSetGlobal, symbol.Index)...)
		case LocalScope:
			c.instructions = append(c.instructions, code.Make(code.OpSetLocal, symbol.Index)...)
		default:
			return fmt.Errorf("local variables not implemented yet")
		}

		return nil

	case *ast.Identifier:
		// 1. 查符号表（局部/全局变量）
		sym, ok := c.symbolTable.Resolve(node.Value)
		if ok {
			// 根据作用域生成对应的指令
			switch sym.Scope {
			case GlobalScope:
				c.instructions = append(c.instructions, code.Make(code.OpGetGlobal, sym.Index)...)
			case LocalScope:
				c.instructions = append(c.instructions, code.Make(code.OpGetLocal, sym.Index)...)
			case FreeScope:
				c.instructions = append(c.instructions, code.Make(code.OpGetFree, sym.Index)...)
			default:
				return fmt.Errorf("local variables not implemented yet")
			}

			return nil
		}

		// 2. 查内置函数
		builtin, ok := builtin.LookUp(node.Value)
		if ok {
			// 将内置函数作为常量存入常量池
			index := c.addConstant(builtin)
			c.instructions = append(c.instructions, code.Make(code.OpConstant, index)...)
			return nil
		}

		return fmt.Errorf("undefined variable: %s", node.Value)

	case *ast.FunctionLiteral:
		// 创建一个新的编译器实例来编译函数体（使用当前符号表作为外部环境）
		// 注意：函数体内部需要局部作用域，所以需要嵌套符号表
		new_compiler := New()
		new_compiler.symbolTable = NewEnclosedSymbolTable(c.symbolTable)

		// 在编译函数体之前，将参数注册到局部符号表
		for _, param := range node.Parameters {
			new_compiler.symbolTable.DefineLocal(param.Value)
		}

		// 编译函数体
		for _, statement := range node.Body.Statements {
			err := new_compiler.Compile(statement)
			if err != nil {
				return err
			}
		}

		// --- 隐式返回处理 ---
		body_len := len(node.Body.Statements)
		if body_len <= 0 {
			// 空函数体 → 返回 null
			new_compiler.instructions = append(new_compiler.instructions, code.Make(code.OpNull)...)
			new_compiler.instructions = append(new_compiler.instructions, code.Make(code.OpReturn)...)
			fmt.Printf("FunctionLiteral body Empty\n")
		} else {
			last_statement := node.Body.Statements[body_len-1]
			switch last_statement.(type) {
			case *ast.ReturnStatement:
				// 已经是显式 return，不额外添加
			case *ast.ExpressionStatement:
				// 表达式值已在栈顶，返回该值
				new_compiler.instructions = append(new_compiler.instructions, code.Make(code.OpReturnVal)...)
			default:
				// let 语句等无值语句 → 返回 null
				new_compiler.instructions = append(new_compiler.instructions, code.Make(code.OpNull)...)
				new_compiler.instructions = append(new_compiler.instructions, code.Make(code.OpReturn)...)
			}
		}

		// 获取自由变量
		numFree := len(new_compiler.symbolTable.freeSymbols)
		// 将编译后的函数对象存入当前编译器的常量池
		fn := &object.CompiledFunction{
			Instructions: new_compiler.instructions,
			Constants:    new_compiler.constants,
			NumLocals:    new_compiler.symbolTable.numDefinitions, // 后续实现局部变量时更新
			NumParams:    len(node.Parameters),
			NumFree:      numFree,
		}

		// 加载自由变量，将入到栈顶。
		for _, freeSymbol := range new_compiler.symbolTable.freeSymbols {
			if LocalScope == freeSymbol.Scope {
				c.instructions = append(c.instructions, code.Make(code.OpGetLocal, freeSymbol.Index)...)
			} else if FreeScope == freeSymbol.Scope {
				newFreeSymbol, ok := c.symbolTable.Resolve(freeSymbol.Name)
				if !ok {
					return fmt.Errorf("Get freeSymbols freeScope wrong")
				}

				c.instructions = append(c.instructions, code.Make(code.OpGetFree, newFreeSymbol.Index)...)
			} else {
				return fmt.Errorf("wrong freeSymbols type")
			}
		}

		fnIndex := c.addConstant(fn)
		c.instructions = append(c.instructions, code.Make(code.OpClosure, fnIndex, numFree)...)
		return nil

	case *ast.ReturnStatement:
		if nil != node.ReturnValue {
			err := c.Compile(node.ReturnValue)
			if nil != err {
				return err
			}

			c.instructions = append(c.instructions, code.Make(code.OpReturnVal)...)
		} else {
			// 没有返回值，压入 null 然后返回
			c.instructions = append(c.instructions, code.Make(code.OpNull)...)
			c.instructions = append(c.instructions, code.Make(code.OpReturn)...)
		}

		return nil

	case *ast.CallExpression:
		// 编译被调用的函数表达式
		err := c.Compile(node.Function)
		if nil != err {
			return err
		}

		// 编译每个参数
		for _, param := range node.Arguments {
			err = c.Compile(param)
			if nil != err {
				return err
			}
		}

		// 生成 OpCall 指令，操作数为参数个数
		c.instructions = append(c.instructions, code.Make(code.OpCall, len(node.Arguments))...)
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
