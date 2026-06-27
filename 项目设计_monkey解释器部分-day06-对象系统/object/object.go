package object

import "fmt"

type objectType string

const (
	INTEGER_OBJ      = "INTEGER"
	BOOLEAN_OBJ      = "BOOLEAN"
	NULL_OBJ         = "NULL"
	RETURN_VALUE_OBJ = "RETURN_VALUE"
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
