# Monkey 解释器

Monkey 编程语言的解释器实现，基于《Writing An Interpreter In Go》（Thorsten Ball）一书，使用 Go 语言从零构建。

## 简介

Monkey 是一种用于学习编程语言设计的教学语言，支持变量、函数、闭包、条件控制、复合数据结构（数组、哈希）等特性。本项目完整实现了其词法分析、语法分析（Pratt 解析器）、求值器以及交互式 REPL 环境。

## 特性

- ✅ 变量绑定（`let`）
- ✅ 整型、布尔、字符串、数组、哈希
- ✅ 算术与比较运算符
- ✅ 函数定义与闭包
- ✅ `if-else` 条件表达式
- ✅ `return` 语句
- ✅ 内置函数（`len`、`first`、`rest`、`push`、`puts`）
- ✅ 交互式 REPL 环境
- ✅ 完整的测试覆盖

## 项目结构
```text
monkey/
├── token/           # Token 定义（词法单元类型、关键字映射）
├── lexer/           # 词法分析器（将源码转换为 Token 流）
├── ast/             # 抽象语法树节点定义
├── parser/          # 语法分析器（Pratt 解析器，生成 AST）
├── object/          # 对象系统（整数、布尔、字符串、数组、哈希、函数、内置函数等）
├── evaluator/       # 求值器（遍历 AST 并执行）
├── repl/            # 交互式 REPL 环境
└── main.go          # 程序入口
```

##核心模块说明
|模块|	职责|	关键实现|
|----|-------|------------|
|Lexer	|将源代码字符串拆分为 Token 序列|	NextToken() 循环读取字符，识别标识符、数字、关键字、运算符等|
|Parser|	将 Token 流组织为 AST|	使用 Pratt 解析器（递归下降 + 优先级表），支持前缀/中缀解析函数|
|AST|	定义程序的结构化表示|	Program 包含 Statements，Statement 和 Expression 接口，具体节点如 LetStatement、InfixExpression 等|
|Object|	表示运行时值|	实现 Object 接口，包含 Integer、Boolean、String、Array、Hash、Function、Builtin 等|
|Evaluator|	执行 AST，产生 Object|	遍历 AST，根据节点类型分发求值，支持 Environment 管理变量作用域和闭包|
|REPL|	交互式环境|	循环读取输入 → Lexer → Parser → Evaluator → 打印结果|

### 架构概览

```text
源代码 → 词法分析 → Token流 → 语法分析 → AST → 求值器 → 结果
                                              ↓
                                        Environment (变量/闭包)
										
```			
```text							
Lexer：将源码转为 Token 序列

Parser：使用 Pratt 解析器构建 AST，支持运算符优先级和结合性

Evaluator：遍历 AST，在 Environment 中查找变量，返回 Object

Object System：运行时值（整数、布尔、字符串、数组、哈希、函数、内置函数）

Environment：支持嵌套作用域，实现闭包
```		

## 关键实现细节
##词法分析（Lexer）
-- 1. 支持单字符和双字符运算符（如 ==、!=）

-- 2. 通过 readIdentifier、readNumber、peekChar 实现前瞻

-- 3. 关键字通过 LookupIdent 映射表区分标识符和关键字

## 语法分析（Parser）
-- 1. Pratt 解析器：每个 Token 类型注册前缀/中缀解析函数

-- 2. 优先级表控制运算符结合性（如 * > +）

-- 3.   支持 let、return、if-else、函数字面量、调用表达式

错误处理：记录未知 Token 或语法错误

## 求值器（Evaluator）
-- 1. Eval 函数：根据 AST 节点类型分发

-- 2. 环境（Environment）：支持变量绑定和作用域链（外链实现闭包）

-- 3.  控制流：if-else 基于条件真值，return 通过 ReturnValue 对象提前退出

-- 4.  函数与闭包：函数定义时保存当前环境，调用时创建新环境并绑定参数

-- 5.  复合类型：字符串、数组、哈希的创建与访问，数组/哈希索引表达式求值

-- 6.  内置函数：len、first、rest、push、puts，在 evalIdentifier 中查表返回

##  REPL
-- 1. 使用 bufio.Scanner 循环读取输入

-- 2. 每行独立执行，保留环境状态（变量可跨行使用）

-- 3. 支持 exit / quit 退出

### 测试情况
```bash	
$ go test ./...
ok      monkey/token     0.001s
ok      monkey/lexer     0.002s
ok      monkey/ast       0.001s
ok      monkey/parser    0.003s
ok      monkey/object    0.001s
ok      monkey/evaluator 0.004s
ok      monkey/repl      0.002s
```	

## 参考资料
-- 1. 《Writing An Interpreter In Go》—— Thorsten Ball

-- 2. Go 官方文档
	
