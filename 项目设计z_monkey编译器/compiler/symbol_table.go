package compiler

type SymbolType int

// SymbolType 表示符号的类型（作用域）
const (
	GlobalScope SymbolType = iota
	LocalScope
	BuiltinScope
)

// Symbol 表示一个符号
type Symbol struct {
	Name  string
	Scope SymbolType
	Index int // 在作用域中的索引
}

// SymbolTable 符号表，支持作用域嵌套
type SymbolTable struct {
	store          map[string]Symbol
	outer          *SymbolTable
	numDefinitions int // 当前作用域定义的符号数量
}

// NewSymbolTable 创建一个新的顶层符号表
func NewSymbolTable() *SymbolTable {
	s := &SymbolTable{
		store:          make(map[string]Symbol),
		outer:          nil,
		numDefinitions: 0,
	}

	return s
}

// NewEnclosedSymbolTable 创建一个嵌套的符号表（用于函数体等局部作用域）
func NewEnclosedSymbolTable(outer *SymbolTable) *SymbolTable {
	s := NewSymbolTable()
	s.outer = outer
	return s
}

// Define 在当前作用域中定义一个符号（变量）
func (s *SymbolTable) Define(name string) Symbol {
	symbol := Symbol{Name: name, Index: s.numDefinitions}
	if nil == s.outer {
		symbol.Scope = GlobalScope
	} else {
		symbol.Scope = LocalScope
	}

	s.store[name] = symbol
	s.numDefinitions++
	return symbol
}

// Resolve 在当前和外部作用域中查找符号
func (s *SymbolTable) Resolve(name string) (Symbol, bool) {
	symbol, ok := s.store[name]
	if !ok && nil != s.outer {
		return s.outer.Resolve(name)
	}

	return symbol, ok
}
