package object

import (
	"bytes"
	"fmt"
	"strings"
)

type objectType string

const (
	INTEGER_OBJ           = "INTEGER"
	BOOLEAN_OBJ           = "BOOLEAN"
	NULL_OBJ              = "NULL"
	RETURN_VALUE_OBJ      = "RETURN_VALUE"
	FUNCTION_OBJ          = "FUNCTION"
	ERROR_OBJ             = "ERROR"
	STRING_OBJ            = "STRING"
	ARRAY_OBJ             = "ARRAY"
	HASH_OBJ              = "HASH"
	BUILTIN_OBJ           = "BUILTIN"
	COMPILED_FUNCTION_OBJ = "COMPILED_FUNCTION"
	CLOSURE_OBJ           = "CLOSURE"
)

type Object interface {
	Type() objectType
	Inspect() string // 返回对象的可读字符串表示，用于调试或 REPL 输出。
}

// ============================================================
// 整数对象
// ============================================================

type Integer struct {
	Value int64
}

func (i *Integer) Type() objectType {
	return INTEGER_OBJ
}

func (i *Integer) Inspect() string {
	return fmt.Sprintf("%d", i.Value)
}

func (i *Integer) HashKey() HashKey {
	return HashKey{Type: i.Type(), Value: uint64(i.Value)}
}

// ============================================================
// 布尔对象
// ============================================================

type Boolean struct {
	Value bool
}

func (b *Boolean) Type() objectType {
	return BOOLEAN_OBJ
}

func (b *Boolean) Inspect() string {
	return fmt.Sprintf("%t", b.Value)
}

func (b *Boolean) HashKey() HashKey {
	val := 0
	if b.Value {
		val = 1
	}

	return HashKey{Type: b.Type(), Value: uint64(val)}
}

// ============================================================
// 空值对象
// ============================================================

type Null struct{}

func (n *Null) Type() objectType {
	return NULL_OBJ
}

func (n *Null) Inspect() string {
	return "null"
}

// ============================================================
// 返回值对象，可以兼容多种类型
// ============================================================
type ReturnValue struct {
	Value Object
}

func (rv *ReturnValue) Type() objectType {
	return RETURN_VALUE_OBJ
}

func (rv *ReturnValue) Inspect() string {
	return rv.Value.Inspect()
}

// ============================================================
// 错误处理对象，可以兼容多种类型
// ============================================================
type Error struct {
	Message string
}

func (e *Error) Type() objectType {
	return ERROR_OBJ
}

func (e *Error) Inspect() string {
	return e.Message
}

// ============================================================
// 字符串对象，可以兼容多种类型
// ============================================================
type String struct {
	Value string
}

func (s *String) Type() objectType {
	return STRING_OBJ
}

func (s *String) Inspect() string {
	return s.Value
}

func (s *String) HashKey() HashKey {
	h := uint64(1469598103934665603) // FNV-1a offset basis
	for _, c := range s.Value {
		h ^= uint64(c)
		h *= 1099511628211
	}

	return HashKey{Type: s.Type(), Value: h}
}

// ============================================================
// 数组对象，可以兼容多种类型
// ============================================================
type Array struct {
	Element []Object
}

func (a *Array) Type() objectType {
	return ARRAY_OBJ
}

func (ao *Array) Inspect() string {
	var out bytes.Buffer
	out.WriteString("[")
	for index, elem := range ao.Element {
		if index > 0 {
			out.WriteString(", ")
		}

		out.WriteString(elem.Inspect())
	}
	out.WriteString("]")
	return out.String()
}

// ============================================================
// 哈希值
// ============================================================
type HashKey struct {
	Type  objectType
	Value uint64
}

// Hasher 接口：可哈希的对象必须实现此方法 FNV 算法可以简单注明是用于确保键的唯一性
type Hasher interface {
	HashKey() HashKey
}

// ============================================================
// 哈希对
// ============================================================
type HashPair struct {
	Key   Object
	Value Object
}

// ============================================================
// 哈希对象
// ============================================================

type Hash struct {
	Pairs map[HashKey]HashPair
}

func (h *Hash) Type() objectType {
	return HASH_OBJ
}

func (h *Hash) Inspect() string {
	pairs := []string{}
	for _, pair := range h.Pairs {
		hashData := pair.Key.Inspect() + ": " + pair.Value.Inspect()
		pairs = append(pairs, hashData)
	}

	var out bytes.Buffer
	out.WriteString("{")
	out.WriteString(strings.Join(pairs, ", "))
	out.WriteString("}")
	return out.String()
}

// ============================================================
// 内置函数对象
// ============================================================

type Builtin struct {
	Fn func(args ...Object) Object
}

func (b *Builtin) Type() objectType {
	return BUILTIN_OBJ
}

func (b *Builtin) Inspect() string {
	return "builtin function"
}

// ============================================================
// 编译后的函数体（字节码 + 元信息），是“函数定义”的静态表示。
// ============================================================
type CompiledFunction struct {
	Instructions []byte        // 编译完成的指令序列集合
	Constants    []interface{} // 常量集合
	NumLocals    int           // 变量个数
	NumParams    int           // 函数参数个数
	NumFree      int           // 自由变量个数
}

func (cf *CompiledFunction) Type() objectType {
	return COMPILED_FUNCTION_OBJ
}

func (cf *CompiledFunction) Inspect() string {
	return fmt.Sprintf("CompiledFunction[%p]", cf)
}

// ============================================================
// 闭包，是“函数定义 + 捕获的变量”的运行时组合，是实际执行时的函数对象。
// 闭包和普通函数的区别在于有没有自由变量，没有自由变量的为Free.
// ============================================================
type Closure struct {
	Fn   *CompiledFunction //指向编译后的函数体
	Free []Object          //Free []Object：捕获的自由变量（闭包捕获的外部变量）
}

func (c *Closure) Type() objectType {
	return CLOSURE_OBJ
}

func (c *Closure) Inspect() string {
	return fmt.Sprintf("Closure[%p]", c)
}
