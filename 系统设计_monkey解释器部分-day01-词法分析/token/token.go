package token

// tokenType 表示词法单元的类型
type TokenType string

// token 结构体表示一个词法单元
type Token struct {
	Type    TokenType
	Literal string // 字面量，例如 "let"、 "5"、 "fn"
}

// 预定义文件的token 类型常量
const (
	// 特殊类型
	ILLEGAL = "ILLEGAL" // 非法字符
	EOF     = "EOF"     // 文件结束

	// 标识符和字面量
	IDENT = "IDENT" // 标识符， 如变量名，函数名
	INT   = "INT"   // 整数

	// 运算符
	ASSIGN   = "="
	PLUS     = "+"
	MINUS    = "-"
	BANG     = "!"
	ASTERISK = "*"
	SLASH    = "/"
	LT       = "<"
	GT       = ">"
	EQ       = "=="
	NOT_EQ   = "!="

	// 分隔符
	COMMA     = ","
	SEMICOLON = ";"
	LPAREN    = "("
	RPAREN    = ")"
	LBRACE    = "{"
	RBRACE    = "}"

	// 关键字
	FUNCTION = "FUNCTION"
	LET      = "LET"
	TRUE     = "TRUE"
	FALSE    = "FALSE"
	IF       = "IF"
	ELSE     = "ELSE"
	RETURN   = "RETURN"
)

// keywords 关键字映射表， 用于判断标识符是否是关键字
var keywords = map[string]TokenType{
	"fn":     FUNCTION,
	"let":    LET,
	"true":   TRUE,
	"false":  FALSE,
	"if":     IF,
	"else":   ELSE,
	"return": RETURN,
}

func LookupIdent(ident string) TokenType {
	token, ok := keywords[ident]
	if ok {
		return token
	}

	return IDENT
}
