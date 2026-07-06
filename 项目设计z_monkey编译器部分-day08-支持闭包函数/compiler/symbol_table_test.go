package compiler

import "testing"

func TestDefineResolve(t *testing.T) {
	global := NewSymbolTable()
	global.Define("a")
	global.Define("b")

	// 测试全局符号解析
	sym, ok := global.Resolve("a")
	if !ok {
		t.Errorf("expected 'a' to be resolved")
	}

	if sym.Index != 0 {
		t.Errorf("expected 'a' index 0, got %d", sym.Index)
	}

	if sym.Scope != GlobalScope {
		t.Errorf("expected 'a' scope GlobalScope, got %d", sym.Scope)
	}

	// 测试嵌套作用域
	local := NewEnclosedSymbolTable(global)
	local.Define("c")

	sym, ok = local.Resolve("c")
	if !ok {
		t.Errorf("expected 'c' to be resolved")
	}

	if sym.Index != 0 {
		t.Errorf("expected 'c' index 0, got %d", sym.Index)
	}

	if sym.Scope != LocalScope {
		t.Errorf("expected 'c' scope LocalScope, got %d", sym.Scope)
	}

	sym, ok = local.Resolve("a")
	if !ok {
		t.Errorf("expected 'a' to be resolved from outer scope")
	}

	if sym.Index != 0 {
		t.Errorf("expected 'a' index 0, got %d", sym.Index)
	}

	if sym.Scope != GlobalScope {
		t.Errorf("expected 'a' scope GlobalScope, got %d", sym.Scope)
	}

	// 测试不存在的符号
	_, ok = local.Resolve("unknown")
	if ok {
		t.Errorf("expected 'unknown' not to be resolved")
	}
}
