markdown

# Day01： monkey解释器  语法分析抽象语法树ast的结构定义

## 核心收获

-- 1.  定义ast语法树的结构

-- 2. 定义结点接口 Node interface. 提供接口函数  TokenLiteral() string   返回该节点关联的 Token 字面量，用于调试 和 String() string  用于打印 AST（调试用）。

-- 3.  定义语句 Statement interface 和 表达式 Expression interface。

-- 4.  语句是控制整个程序的流程和执行动作的。 而表达式产生一个值, 同时表达式是挂在语句下面的一个结点。

-- 5.  monkey设计中有4个语句， 这边定义了他们的实现并且返回Node中的接口和区分语句的接口。4个语句分别是 
	
	LetStatement 绑定变量语句， 
	
	ReturnStatement 返回值语句， 
	
	ExpressionStatement 表示式语句(将表达式包装成语句变成语句列表里面的一条)， 
	
	BlockStatement 程序块语句(程序块语句有多条单个语句构成)。
	

-- 5.  monkey设计中有多个表达式的类型， 这边定义了他们的实现并且返回Node中的接口和区分表达式的接口。多个表达式分别是 

	Identifier 标识符表达式（变量名、函数名等）

	IntegerLiteral 表示整数字面量
	
	Boolean 表示布尔字面量
	
	StringLiteral 表示字符串字面量
	
	ArrayLiteral 表示数组字面量
	
	HashLiteral 表示哈希表字面量
	
	FunctionLiteral 表示函数字面量: fn(<params>) <block>
	
	PrefixExpression 表示前缀表达式: <operator><expression>
	
	InfixExpression 表示中缀表达式: <expression><operator><expression>
	
	IfExpression 表示条件表达式: if (<condition>) <consequence> [else <alternative>]
	
	CallExpression 表示函数调用: <function>(<arguments>)
	
	IndexExpression 表示索引表达式，如 arr[0] 或 hash["key"]
	
## 测试
-- 单元测试一切正常。


