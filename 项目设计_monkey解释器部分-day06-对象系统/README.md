markdown

# Day06： monkey解释器 对象系统和相关求值

## 核心收获

-- 1. 定义一个对象系统，求值器求值需要返回一个对象。定义一个根对象，其他对象都需要实现这个接口。
```go
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

```


-- 2. 实现对应的求值器，这个是解释器返回最终执行结果的核心。执行整个程序的根节点ast.program
```go
func evalProgram(program *ast.Program) object.Object {
	var result object.Object
	for _, statement := range program.StatementList {
		result = Eval(statement)
		returnVal, ok := result.(*object.ReturnValue)
		if ok && nil != returnVal {
			return returnVal.Value
		}
	}

	return result
}

```

-- 3. 其余各自表达式和语句都有各自不同的求值和计算。



## 测试
-- 单元测试一切正常。


