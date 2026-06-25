package ast

import (
	"bytes"
	"monkey/token"
	"strings"
)

// ============================================================================
// 基础接口
// ============================================================================

// Node 是所有ast节点的接口
type Node interface {
	TokenLiteral() string // 返回该节点关联的 Token 字面量，用于调试
	String() string       // 用于打印 AST（调试用）
}

// Statement 语句节点接口（程序中的声明、控制流等）
type Statement interface {
	Node
	statementNode() // 占位方法，用于区分语句和表达式
}

// Expression 表达式节点接口（求值产生值）
type Expression interface {
	Node
	expressionNode() // 占位方法，用于区分语句和表达式
}

// ============================================================================
// Program 根节点
// ============================================================================
// Program 是整个程序的根节点，包含所有语句
type Program struct {
	StatementList []Statement
}

func (p *Program) TokenLiteral() string {
	if len(p.StatementList) > 0 {
		return p.StatementList[0].TokenLiteral()
	}

	return ""
}

func (p *Program) String() string {
	var out bytes.Buffer
	for _, statement := range p.StatementList {
		out.WriteString(statement.String())
	}

	return out.String()
}

// ============================================================================
// 语句节点
// ============================================================================

// LetStatement 表示变量声明语句: let <identifier> = <expression>;
type LetStatement struct {
	Token token.Token // token.LET
	Name  *Identifier // 变量名（标识符表达式）
	Value Expression  // 初始值表达式
}

func (ls *LetStatement) statementNode() {

}

func (ls *LetStatement) TokenLiteral() string {
	return ls.Token.Literal
}

func (ls *LetStatement) String() string {
	var out bytes.Buffer
	out.WriteString(ls.TokenLiteral())
	out.WriteString(" ")
	out.WriteString(ls.Name.String())
	out.WriteString(" = ")
	if nil != ls.Value {
		out.WriteString(ls.Value.String())
	}

	out.WriteString(";")
	return out.String()
}

// ReturnStatement 表示返回语句: return <expression>;
type ReturnStatement struct {
	Token       token.Token // token.RETURN
	ReturnValue Expression
}

func (rs *ReturnStatement) statementNode() {

}

func (rs *ReturnStatement) TokenLiteral() string {
	return rs.Token.Literal
}

func (rs *ReturnStatement) String() string {
	var out bytes.Buffer
	out.WriteString(rs.TokenLiteral())
	out.WriteString(" ")
	if nil != rs.ReturnValue {
		out.WriteString(rs.ReturnValue.String())
	}

	out.WriteString(";")
	return out.String()
}

// ExpressionStatement 表示一个仅包含表达式的语句: <expression>;
type ExpressionStatement struct {
	Token      token.Token // 表达式的第一个 Token
	Expression Expression
}

func (es *ExpressionStatement) statementNode() {

}

func (es *ExpressionStatement) TokenLiteral() string {
	return es.Token.Literal
}

func (es *ExpressionStatement) String() string {
	if nil != es.Expression {
		return es.Expression.String()
	}

	return ""
}

// BlockStatement 表示代码块: { <statements> }
type BlockStatement struct {
	Token      token.Token // '{' token
	Statements []Statement
}

func (bs *BlockStatement) statementNode() {

}

func (bs *BlockStatement) TokenLiteral() string {
	return bs.Token.Literal
}

func (bs *BlockStatement) String() string {
	var out bytes.Buffer
	out.WriteString("{")
	for _, statement := range bs.Statements {
		out.WriteString(statement.String())
	}

	out.WriteString("}")
	return out.String()
}

// ============================================================================
// 表达式节点
// ============================================================================

// Identifier 表示标识符表达式（变量名、函数名等）
type Identifier struct {
	Token token.Token // token.IDENT
	Value string
}

func (i *Identifier) expressionNode() {

}

func (i *Identifier) TokenLiteral() string {
	return i.Token.Literal
}

func (i *Identifier) String() string {
	return i.Value
}

// IntegerLiteral 表示整数字面量
type IntegerLiteral struct {
	Token token.Token // token.INT
	Value int64
}

func (il *IntegerLiteral) expressionNode() {

}

func (il *IntegerLiteral) TokenLiteral() string {
	return il.Token.Literal
}

func (il *IntegerLiteral) String() string {
	return il.Token.Literal
}

// Boolean 表示布尔字面量
type Boolean struct {
	Token token.Token // token.TRUE 或 token.FALSE
	Value bool
}

func (b *Boolean) expressionNode() {

}

func (b *Boolean) TokenLiteral() string {
	return b.Token.Literal
}

func (b *Boolean) String() string {
	return b.Token.Literal
}

// PrefixExpression 表示前缀表达式: <operator><expression>
type PrefixExpression struct {
	Token    token.Token // 前缀运算符 token，如 '!', '-'
	Operator string
	Right    Expression
}

func (pe *PrefixExpression) expressionNode() {

}

func (pe *PrefixExpression) TokenLiteral() string {
	return pe.Token.Literal
}

func (pe *PrefixExpression) String() string {
	var out bytes.Buffer
	out.WriteString("(")
	out.WriteString(pe.Operator)
	out.WriteString(pe.Right.String())
	out.WriteString(")")
	return out.String()
}

// InfixExpression 表示中缀表达式: <expression><operator><expression>
type InfixExpression struct {
	Token    token.Token // 运算符 token，如 '+', '-', '*', '/', '<', '>', '==', '!='
	Left     Expression
	Operator string
	Right    Expression
}

func (ie *InfixExpression) expressionNode() {

}

func (ie *InfixExpression) TokenLiteral() string {
	return ie.Token.Literal
}

func (ie *InfixExpression) String() string {
	var out bytes.Buffer
	out.WriteString("(")
	out.WriteString(ie.Left.String())
	out.WriteString(" ")
	out.WriteString(ie.Operator)
	out.WriteString(" ")
	out.WriteString(ie.Right.String())
	out.WriteString(")")
	return out.String()
}

// IfExpression 表示条件表达式: if (<condition>) <consequence> [else <alternative>]
type IfExpression struct {
	Token       token.Token // 'if' token
	Condition   Expression
	Consequence *BlockStatement
	Alternative *BlockStatement
}

func (ie *IfExpression) expressionNode() {

}

func (ie *IfExpression) TokenLiteral() string {
	return ie.Token.Literal
}

func (ie *IfExpression) String() string {
	var out bytes.Buffer
	out.WriteString("if")
	out.WriteString(ie.Condition.String())
	out.WriteString(" ")
	out.WriteString(ie.Consequence.String())
	if nil != ie.Alternative {
		out.WriteString("else")
		out.WriteString(ie.Alternative.String())
	}

	return out.String()
}

// FunctionLiteral 表示函数字面量: fn(<params>) <block>
type FunctionLiteral struct {
	Token      token.Token // 'fn' token
	Parameters []*Identifier
	Body       *BlockStatement
}

func (fl *FunctionLiteral) expressionNode() {

}

func (fl *FunctionLiteral) TokenLiteral() string {
	return fl.Token.Literal
}

func (fl *FunctionLiteral) String() string {
	params := []string{}
	for _, param := range fl.Parameters {
		params = append(params, param.String())
	}

	var out bytes.Buffer
	out.WriteString(fl.TokenLiteral())
	out.WriteString("(")
	out.WriteString(strings.Join(params, ", "))
	out.WriteString(") ")
	out.WriteString(fl.Body.String())
	return out.String()
}

// CallExpression 表示函数调用: <function>(<arguments>)
type CallExpression struct {
	Token     token.Token // '(' token
	Function  Expression  // 被调用的表达式（标识符或函数字面量）
	Arguments []Expression
}

func (ce *CallExpression) expressionNode() {

}

func (ce *CallExpression) TokenLiteral() string {
	return ce.Token.Literal
}

func (ce *CallExpression) String() string {
	args := []string{}
	for _, argument := range ce.Arguments {
		args = append(args, argument.String())
	}

	var out bytes.Buffer
	out.WriteString(ce.Function.String())
	out.WriteString("(")
	out.WriteString(strings.Join(args, ", "))
	out.WriteString(")")
	return out.String()
}

// ArrayLiteral 表示函数调用: 表示数组字面量，如 [1, 2, 3]
type ArrayLiteral struct {
	Token    token.Token
	Elements []Expression
}

func (al *ArrayLiteral) expressionNode() {

}

func (al *ArrayLiteral) TokenLiteral() string {
	return al.Token.Literal
}

func (al *ArrayLiteral) String() string {
	elements := []string{}
	for _, tmpElemnet := range al.Elements {
		elements = append(elements, tmpElemnet.String())
	}

	var out bytes.Buffer
	out.WriteString("[]")
	out.WriteString(strings.Join(elements, ", "))
	out.WriteString("]")
	return out.String()
}

// IndexExpression 表示索引表达式，如 arr[0] 或 hash["key"]
type IndexExpression struct {
	Token token.Token
	Left  Expression
	Index Expression
}

func (ie *IndexExpression) expressionNode() {

}

func (ie *IndexExpression) TokenLiteral() string {
	return ie.Token.Literal
}

func (ie *IndexExpression) String() string {
	var out bytes.Buffer
	out.WriteString("(")
	out.WriteString(ie.Left.String())
	out.WriteString("[")
	out.WriteString(ie.Index.String())
	out.WriteString("])")
	return out.String()
}

// HashLiteral 表示哈希字面量，如 {"key": 1, "value": 2}
type HashLiteral struct {
	Token token.Token
	Pairs map[Expression]Expression
}

func (hl *HashLiteral) expressionNode() {

}

func (hl *HashLiteral) TokenLiteral() string {
	return hl.Token.Literal
}

func (hl *HashLiteral) String() string {
	pairString := []string{}
	for key, val := range hl.Pairs {
		pairString = append(pairString, key.String()+": "+val.String())
	}

	var out bytes.Buffer
	out.WriteString("{")
	out.WriteString(strings.Join(pairString, ", "))
	out.WriteString("}")
	return out.String()
}

// StringLiteral 表示字符串字面量，如 "hello"
type StringLiteral struct {
	Token token.Token
	value string
}

func (sl *StringLiteral) expressionNode() {

}

func (sl *StringLiteral) TokenLiteral() string {
	return sl.Token.Literal
}

func (sl *StringLiteral) String() string {
	return sl.Token.Literal
}
