package builtin

import (
	"fmt"
	"monkey/object"
)

// ============================================================
// 内置函数表
// ============================================================
var builtin_table = map[string]*object.Builtin{
	"len":   &object.Builtin{Fn: builtinLen},
	"first": &object.Builtin{Fn: builtinFirst},
	"rest":  &object.Builtin{Fn: builtinRest},
	"push":  &object.Builtin{Fn: builtinPush},
	"puts":  &object.Builtin{Fn: builtinPuts},
}

func LookUp(name string) (*object.Builtin, bool) {
	builtinObj, ok := builtin_table[name]
	return builtinObj, ok
}

func builtinLen(args ...object.Object) object.Object {
	if len(args) != 1 {
		return newError("wrong number of arguments, got=%d, want=1", len(args))
	}

	switch arg := args[0].(type) {
	case *object.String:
		return &object.Integer{Value: int64(len(arg.Value))}
	case *object.Array:
		return &object.Integer{Value: int64(len(arg.Element))}
	case *object.Hash:
		return &object.Integer{Value: int64(len(arg.Pairs))}
	default:
		return newError("argument to `len` not supported, got %s", arg.Type())
	}
}

func builtinFirst(args ...object.Object) object.Object {
	if len(args) != 1 {
		return newError("wrong number of arguments, got=%d, want=1", len(args))
	}

	argType := args[0].Type()
	if object.ARRAY_OBJ != argType {
		return newError("argument to `first` must be Array, got=%s", argType)
	}

	arr := args[0].(*object.Array)
	if len(arr.Element) > 0 {
		return arr.Element[0]
	}

	return &object.Null{}
}

func builtinRest(args ...object.Object) object.Object {
	if len(args) != 1 {
		return newError("wrong number of arguments, got=%d, want=1", len(args))
	}

	argType := args[0].Type()
	if object.ARRAY_OBJ != argType {
		return newError("argument to `first` must be Array, got=%s", argType)
	}

	array := args[0].(*object.Array)
	length := len(array.Element)
	if length > 0 {
		newElement := make([]object.Object, length-1)
		copy(newElement, array.Element[1:length])
		return &object.Array{Element: newElement}
	}

	return &object.Null{}
}

func builtinPush(args ...object.Object) object.Object {
	if len(args) != 2 {
		return newError("wrong number of arguments, got=%d, want=1", len(args))
	}

	argType := args[0].Type()
	if object.ARRAY_OBJ != argType {
		return newError("argument to `first` must be Array, got=%s", argType)
	}

	array := args[0].(*object.Array)
	oldLength := len(array.Element)

	newElement := make([]object.Object, oldLength+1)
	copy(newElement, array.Element)
	newElement[oldLength] = args[1]
	return &object.Array{Element: newElement}
}

func builtinPuts(args ...object.Object) object.Object {
	for _, arg := range args {
		fmt.Println(arg.Inspect())
	}

	return &object.Null{}
}

// newError 创建错误对象(先简单实现，后续完善)
func newError(format string, a ...interface{}) object.Object {
	// 返回 NULL 或特殊错误对象
	return &object.Error{Message: fmt.Sprintf(format, a...)}
}
