package object

import "monkey/ast"

// Function 表示函数对象
/* FunctionLiteral 表示函数字面量: fn(<params>) <block>
type FunctionLiteral struct {
	Token      token.Token // 'fn' token
	Parameters []*Identifier
	Body       *BlockStatement
}
*/
type Function struct {
	Parameters []*ast.Identifier
	Body       *ast.BlockStatement
	Env        *Environment // 捕获的环境（闭包）
}

func (f *Function) Type() objectType {
	return FUNCTION_OBJ
}

func (f *Function) Inspect() string {
	return "fn(" + f.parametersString() + ") " + f.Body.String()
}

func (f *Function) parametersString() string {
	var out string
	for i, param := range f.Parameters {
		if i > 0 {
			out += ", "
		}

		out += param.String()
	}

	return out
}
