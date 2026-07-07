package lexer

import (
	"monkey/token"
)

// 辅助函数， 判断是否是字母(含下划线)
func isLetter(ch byte) bool {
	return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || ch == '_'
}

// 辅助函数， 判断是否是数字
func isDigit(ch byte) bool {
	return ch >= '0' && ch <= '9'
}

// 词法分析器
type Lexer struct {
	input        string //
	positon      int    // 当前字符的位置， 指向ch
	readPosition int    // 正在读取的位置， position+1
	ch           byte   // 当前正在查看的字符
}

// New创建一个新的lexer的实例
func New(input string) *Lexer {
	l := &Lexer{input: input}
	l.readChar()
	return l
}

// 读取一个字符
func (l *Lexer) readChar() {
	if l.readPosition >= len(l.input) {
		l.ch = 0
	} else {
		l.ch = l.input[l.readPosition]
	}

	l.positon = l.readPosition
	l.readPosition += 1
}

// 获取下一个字符的byte内容
func (l *Lexer) peekChar() byte {
	if l.readPosition >= len(l.input) {
		return 0
	}

	return l.input[l.readPosition]
}

// 跳过空格 tab键 和 换行符
func (l *Lexer) skipWhiteSpace() {
	for ' ' == l.ch || '\t' == l.ch || '\r' == l.ch || '\n' == l.ch {
		l.readChar()
	}
}

// 读取标识符，这边标识符的定义是 a——z, A-Z和-三种字符组成的。
// 可以修改readIdentifier 支持其他规则
func (l *Lexer) readIdentifier() string {
	start_position := l.positon
	for isLetter(l.ch) {
		l.readChar()
	}

	return l.input[start_position:l.positon]
}

// 支持数字型常量
func (l *Lexer) readNumber() string {
	start_positon := l.positon
	for isDigit(l.ch) {
		l.readChar()
	}

	return l.input[start_positon:l.positon]
}

// 支持字符串常量
func (l *Lexer) readString() string {
	start_positon := l.positon + 1
	for {
		l.readChar()
		if '"' == l.ch || 0 == l.ch {
			break
		}
	}

	return l.input[start_positon:l.positon]
}

// 做词法分析，获取下一个token. 根据开头的字符进行token类型的预判
// 然后进行整个token类型内容的完整获取。再进行下一个开头字符的token处理直到字节流结束。
func (l *Lexer) NextToken() token.Token {
	readNextChar := true
	var tok token.Token
	l.skipWhiteSpace()
	switch l.ch {
	case '+':
		tok = token.Token{Type: token.PLUS, Literal: string(l.ch)}
	case '-':
		tok = token.Token{Type: token.MINUS, Literal: string(l.ch)}
	case '*':
		tok = token.Token{Type: token.ASTERISK, Literal: string(l.ch)}
	case '/':
		tok = token.Token{Type: token.SLASH, Literal: string(l.ch)}
	case '<':
		tok = token.Token{Type: token.LT, Literal: string(l.ch)}
	case '>':
		tok = token.Token{Type: token.GT, Literal: string(l.ch)}
	case ',':
		tok = token.Token{Type: token.COMMA, Literal: string(l.ch)}
	case ';':
		tok = token.Token{Type: token.SEMICOLON, Literal: string(l.ch)}
	case '(':
		tok = token.Token{Type: token.LPAREN, Literal: string(l.ch)}
	case ')':
		tok = token.Token{Type: token.RPAREN, Literal: string(l.ch)}
	case '{':
		tok = token.Token{Type: token.LBRACE, Literal: string(l.ch)}
	case '}':
		tok = token.Token{Type: token.RBRACE, Literal: string(l.ch)}
	case '[':
		tok = token.Token{Type: token.LBRACKET, Literal: string(l.ch)}
	case ']':
		tok = token.Token{Type: token.RBRACKET, Literal: string(l.ch)}
	case ':':
		tok = token.Token{Type: token.COLON, Literal: string(l.ch)}
	case '"':
		literal := l.readString()
		tok = token.Token{Type: token.STRING, Literal: literal}
	case 0:
		tok = token.Token{Type: token.EOF, Literal: string(l.ch)}
		readNextChar = false
	case '!':
		next_ch := l.peekChar()
		if '=' == next_ch {
			pre_ch := l.ch
			l.readChar()
			literal := string(pre_ch) + string(l.ch)
			tok = token.Token{Type: token.NOT_EQ, Literal: literal}
		} else {
			tok = token.Token{Type: token.BANG, Literal: string(l.ch)}
		}

	case '=':
		next_ch := l.peekChar()
		if '=' == next_ch {
			pre_ch := l.ch
			l.readChar()
			literal := string(pre_ch) + string(l.ch)
			tok = token.Token{Type: token.EQ, Literal: literal}
		} else {
			tok = token.Token{Type: token.ASSIGN, Literal: string(l.ch)}
		}

	default:
		// 是否是以字母或者-打头，是的话则要么是标识符要么是关键字
		if isLetter(l.ch) {
			literal := l.readIdentifier()
			tokenType := token.LookupIdent(literal)
			tok = token.Token{Type: tokenType, Literal: literal}
			readNextChar = false
		} else if isDigit(l.ch) {
			literal := l.readNumber()
			tok = token.Token{Type: token.INT, Literal: literal}
			readNextChar = false
		} else {
			tok = token.Token{Type: token.ILLEGAL, Literal: string(l.ch)}
		}
	}

	if readNextChar {
		l.readChar()
	}

	return tok
}

// Tokens 返回词法分析器生成的所有 Token 切片（包括 EOF）
func (l *Lexer) Tokens() []token.Token {
	var allTokens []token.Token
	for {
		curToken := l.NextToken()
		allTokens = append(allTokens, curToken)
		if token.EOF == curToken.Type {
			break
		}
	}

	return allTokens
}
