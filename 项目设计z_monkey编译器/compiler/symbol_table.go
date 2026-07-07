package compiler

type SymbolType int

// SymbolType 表示符号的类型（作用域）
const (
	GlobalScope SymbolType = iota
	LocalScope
	FreeScope
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
	numDefinitions int      // 当前作用域定义的符号数量
	freeSymbols    []Symbol // 存储捕获的自由变量,用于生成 CompiledFunction.NumFree的个数，同时给传递给Object.Closure中的free
}

// NewSymbolTable 创建一个新的顶层符号表
func NewSymbolTable() *SymbolTable {
	s := &SymbolTable{
		store:          make(map[string]Symbol),
		outer:          nil,
		numDefinitions: 0,
		freeSymbols:    []Symbol{},
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

// Define 在当前作用域中定义一个符号（变量）
func (s *SymbolTable) DefineLocal(name string) Symbol {
	symbol := Symbol{Name: name, Index: s.numDefinitions, Scope: LocalScope}
	s.store[name] = symbol
	s.numDefinitions++
	return symbol
}

func (s *SymbolTable) DefineFree(original Symbol) Symbol {
	// 将原始符号（来自外部作用域）存入 freeSymbols 列表
	s.freeSymbols = append(s.freeSymbols, original)

	// 在当前符号表注册一个新符号，Scope 为 FreeScope，Index 指向 freeSymbols 中的位置
	symbol := Symbol{Name: original.Name, Index: len(s.freeSymbols) - 1, Scope: FreeScope}
	s.store[original.Name] = symbol

	return symbol
}

// Resolve 在当前和外部作用域中查找符号
func (s *SymbolTable) Resolve(name string) (Symbol, bool) {
	symbol, ok := s.store[name]
	if ok {
		return symbol, ok
	}

	if nil != s.outer {
		outerSym, found := s.outer.Resolve(name)
		if found {
			if GlobalScope == outerSym.Scope || BuiltinScope == outerSym.Scope {
				return outerSym, true
			}

			// 否则是外层局部变量或自由变量，需要捕获为自由变量
			freeSym := s.DefineFree(outerSym)
			return freeSym, true
		}
	}

	return Symbol{}, false
}

// GetFreeSymbols 返回自由变量列表
func (s *SymbolTable) GetFreeSymbols() []Symbol {
	return s.freeSymbols
}
