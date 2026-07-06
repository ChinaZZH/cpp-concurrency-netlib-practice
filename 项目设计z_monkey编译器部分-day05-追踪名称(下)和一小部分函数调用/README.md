markdown

# Day05 monkey编译器  追踪名称(下)

## 核心收获

-- 5.101.  定义frame支持每个函数调用环境，并且vm会定义 frame数组来定义  函数调用栈帧，标注当前所执行的函数frameIdex.
```go
// Frame 表示一个函数调用帧
type Frame struct {
	fn     *object.Closure // 指向被调用的闭包
	ip     int             // 指令指针（当前执行位置）
	locals []object.Object //局部变量
}

type Closure struct {
	Fn   *CompiledFunction //指向编译后的函数体
	Free []Object          //Free []Object：捕获的自由变量（闭包捕获的外部变量）
}

type CompiledFunction struct {
	Instructions []byte
	Constants    []interface{}
	NumLocals    int
	NumParams    int
	NumFree      int
}

type VM struct {
	globals   []object.Object // 全局变量存储
	frames    []*Frame        // 调用帧栈
	frameIdex int             // 当前帧索引
	stack     []object.Object // 操作数栈
	sp        int             // 栈指针
	framePool sync.Pool       // 内存池
}
```

-- 5.102.  编译函数字面量的时候需要支持隐式返回，通过return指令来处理函数帧栈的出栈操作。
```go
		body_len := len(node.Body.Statements)
		if body_len <= 0 {
			// 空函数体 → 返回 null
			new_compiler.instructions = append(new_compiler.instructions, code.Make(code.OpNull)...)
			new_compiler.instructions = append(new_compiler.instructions, code.Make(code.OpReturn)...)
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
```

-- 5.103.  编译return指令面对return ; 的时候需要插入一条code.OpNull， 这个指令就是往操作数栈里面插入null 对象。

				monkey设计任何表达式都需要返回值， 支持用if语句，let语句各种语句使用嵌入函数调用。  插入 OpNull 是为了保证虚拟机执行模型的一致性。
				
-- 5.104.  编译函数字面量的时候就相当于生成一个常量，只不过这个常量需要额外处理一些其他内容，所以用code.OpClosure指令和code.OpConstant和常量处理区分开来。


## 整个程序的核心

