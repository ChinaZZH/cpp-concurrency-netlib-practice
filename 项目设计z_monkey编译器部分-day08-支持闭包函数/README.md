markdown

# Day08 monkey编译器  支持闭包

## 核心收获

-- 1. 闭包支持自由变量，也就是从上级函数的变量并且不是通过函数参数传递的。

-- 2. 符号表在当前store取不到数据的时候，用外层的outer.store取到数据的时候要判断是否是全局或者内置内置标识符，如果是的话直接返回，不是的话则设置为自由变量。

-- 3. 在编译函数字面量的时候，需要先生成将自由变量放入vm操作数栈的指令。
```go
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
```

-- 4. vm运行code.OpClosure闭包指令的时候，需要先从操作数栈中获取到自由变量，同时赋值给闭包对象。

## 整个程序的核心

