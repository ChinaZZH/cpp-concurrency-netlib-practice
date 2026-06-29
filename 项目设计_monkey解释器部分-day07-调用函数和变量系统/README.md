markdown

# Day07： monkey解释器 求值器环境绑定

## 核心收获

-- 1. 增加环境绑定，环境里面存储着当前的标识符的名称和对应具体的对象数值，同时还支持闭包(查找上一层环境)。环境的定义是这样的。
```go
// Environment 表示一个作用域环境
type Environment struct {
	store map[string]Object
	outer *Environment // 外层环境，用于作用域链
}

```

-- 2. 支持对应环境的标识符查找，标识符设置，还有将当前环境作为新环境的上一层环境 。

-- 3. 有了环境后，就可以支持letStatement绑定变量语句，标识符具体对象查找， 函数定义语句，函数调用语句。


## 测试
-- 单元测试一切正常。


