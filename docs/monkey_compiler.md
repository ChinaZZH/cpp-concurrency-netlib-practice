# Monkey 编译器与虚拟机

Monkey 编程语言的**字节码编译器 + 栈式虚拟机**实现。本项目是 Monkey 解释器的进阶版本，将 AST 编译为字节码并由虚拟机执行，性能更高，更接近真实语言的实现方式。

## 简介

Monkey 是一种用于学习编程语言设计的教学语言，支持变量、函数、闭包、条件控制、复合数据结构（数组、哈希）等特性。本项目从零实现了其**编译器 + 虚拟机**工具链，包含完整的词法分析、语法分析、编译、字节码生成和虚拟机执行流程。

### 与解释器的关系

- **解释器**（Evaluator）：直接遍历 AST 求值，实现简单，适合教学
- **编译器 + 虚拟机**（Compiler + VM）：将 AST 编译为字节码，由虚拟机执行，性能更高，更接近真实语言的实现方式

本项目实现了后者，且与解释器共享 Lexer、Parser 和 AST 模块。

## 特性

- ✅ 词法分析（Lexer）与语法分析（Pratt 解析器）
- ✅ 抽象语法树（AST）构建
- ✅ 变量绑定与环境管理（SymbolTable）
- ✅ 函数定义与调用（含闭包、递归）
- ✅ 条件控制（`if-else`）
- ✅ 复合数据类型：字符串、数组、哈希
- ✅ 内置函数：`len`、`first`、`rest`、`push`、`puts`
- ✅ 字节码编译器（Compiler）
- ✅ 栈式虚拟机（VM）
- ✅ 交互式 REPL 环境
- ✅ 帧池优化与指令缓存
- ✅ 完整的测试覆盖

## 项目结构
```text
monkey/
├── code/ # 字节码指令定义与编码器
│ └── code.go
├── compiler/ # 字节码编译器
│ ├── compiler.go
│ └── symbol_table.go
├── vm/ # 栈式虚拟机
│ └── vm.go
├── token/ # Token 定义
├── lexer/ # 词法分析器
├── ast/ # 抽象语法树节点
├── parser/ # Pratt 解析器
├── object/ # 运行时对象系统
├── repl/ # 交互式 REPL
└── main.go # 程序入口
```
REPL 交互
在 REPL 中输入 Monkey 代码：
```text
>> let add = fn(x, y) { x + y; };
>> add(2, 3);
5
>> len("hello");
5
>> [1, 2, 3][0];
1
>> {"name": "Monkey"}["name"];
Monkey
```

编译模式（查看字节码）
在 REPL 中输入 :c 前缀查看编译后的字节码：
```text
>> :c fn(x) { x + 1; }
=====Bytecode=======
0000: OpClosure 0
=====Constants======
0: CompiledFunction[...]
   Function (params=1, locals=0):
0000: OpGetLocal 0
0003: OpConstant 0
0006: OpAdd
0007: OpReturnValue
```

运行测试
```text
go test ./...
```

## 核心模块说明
|模块|	职责|
|----|-------|
|Lexer	|将源码转为 Token 序列|
|Parser|	使用 Pratt 解析器构建 AST，支持运算符优先级|
|AST|	定义程序的结构化表示|	
|Object|	表示运行时值|	
|REPL|	交互式环境|	
|Compiler|	遍历 AST，生成字节码指令序列，管理常量池与符号表|
|VM|	执行字节码，管理帧栈、操作数栈、全局变量与闭包捕获|
|SymbolTable|	管理变量名 → 索引映射，支持作用域嵌套与自由变量捕获|
|Frame|	函数调用的执行上下文，包含局部变量、指令指针和闭包引用|

### 架构概览

```text
源码 → Lexer → Token流 → Parser → AST → Compiler → 字节码 → VM → 执行结果
																			↓
																SymbolTable (变量绑定)
																			↓
																	VM 执行 → Frame (运行时调用栈) 和stack(操作数栈)→ 执行结果
										
```			


## 技术亮点
-- 1. Pratt 解析器：通过优先级表控制运算符结合性，优雅处理表达式解析

-- 2. 符号表与作用域链：支持全局/局部作用域嵌套，实现闭包捕获自由变量

-- 3. 帧管理：基于帧（Frame）的调用栈，支持函数调用、参数传递和递归

-- 4. 闭包实现：通过 OpGetFree 指令从闭包 Free 数组读取捕获的外部变量

-- 5. 帧池优化：使用 sync.Pool 复用 Frame 对象，减少函数调用时的内存分配

### 测试情况
```bash	
$ go test ./...
ok      monkey/token     0.001s
ok      monkey/lexer     0.002s
ok      monkey/ast       0.001s
ok      monkey/parser    0.003s
ok      monkey/object    0.001s
ok      monkey/compiler  0.003s
ok      monkey/vm        0.005s
ok      monkey/repl      0.002s
```	

## 参考资料
-- 1.《Writing An Interpreter In Go》—— Thorsten Ball

-- 2.《Writing A Compiler In Go》—— Thorsten Ball
	
