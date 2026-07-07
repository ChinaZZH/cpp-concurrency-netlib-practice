package object

// Environment 表示一个作用域环境
type Environment struct {
	store map[string]Object // 相当于编译器的符号表，存储变量或者函数的表示以及对应的具体字面值的object
	outer *Environment      // 外层环境，用于作用域链
}

// NewEnvironment 创建一个新的顶层环境
func NewEnvironment() *Environment {
	tmp_store := make(map[string]Object)
	return &Environment{store: tmp_store, outer: nil}
}

// NewEnclosedEnvironment 创建一个嵌套环境
func NewEnclosedEnvironment(outer *Environment) *Environment {
	env := NewEnvironment()
	env.outer = outer
	return env
}

// Get 在环境中查找变量，如果不存在则在外层查找
func (e *Environment) Get(name string) (Object, bool) {
	obj, ok := e.store[name]
	if !ok && nil != e.outer {
		return e.outer.Get(name)
	}

	return obj, ok
}

// Set 在当前环境中绑定变量
func (e *Environment) Set(name string, value Object) Object {
	e.store[name] = value
	return value
}
