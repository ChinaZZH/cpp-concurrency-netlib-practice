package lexer

import (
	"fmt"
	"monkey/token"
	"testing"
)

func TestNextToken(t *testing.T) {
	input := `let five = 5; let ten = 10;
	let add = fn(x, y) { x+y; };
	let result = add(five, ten);
	!-/*5;
	5 < 10 > 5;
	
	if (5 < 10) { return true; }
	else { return false; }
	
	10 == 10; 10 != 9; let just_go = "abcde"; `

	test := []struct {
		expectedType   token.TokenType
		expectedLitera string
	}{
		// let five = 5;
		{token.LET, "let"},
		{token.IDENT, "five"},
		{token.ASSIGN, "="},
		{token.INT, "5"},
		{token.SEMICOLON, ";"},

		// let ten = 10;
		{token.LET, "let"},
		{token.IDENT, "ten"},
		{token.ASSIGN, "="},
		{token.INT, "10"},
		{token.SEMICOLON, ";"},

		// let add = fn(x, y) { x + y; };
		{token.LET, "let"},
		{token.IDENT, "add"},
		{token.ASSIGN, "="},
		{token.FUNCTION, "fn"},
		{token.LPAREN, "("},
		{token.IDENT, "x"},
		{token.COMMA, ","},
		{token.IDENT, "y"},
		{token.RPAREN, ")"},
		{token.LBRACE, "{"},
		{token.IDENT, "x"},
		{token.PLUS, "+"},
		{token.IDENT, "y"},
		{token.SEMICOLON, ";"},
		{token.RBRACE, "}"},
		{token.SEMICOLON, ";"},

		// let result = add(five, ten);
		{token.LET, "let"},
		{token.IDENT, "result"},
		{token.ASSIGN, "="},
		{token.IDENT, "add"},
		{token.LPAREN, "("},
		{token.IDENT, "five"},
		{token.COMMA, ","},
		{token.IDENT, "ten"},
		{token.RPAREN, ")"},
		{token.SEMICOLON, ";"},

		// !-/*5;
		{token.BANG, "!"},
		{token.MINUS, "-"},
		{token.SLASH, "/"},
		{token.ASTERISK, "*"},
		{token.INT, "5"},
		{token.SEMICOLON, ";"},

		// 5 < 10 > 5;
		{token.INT, "5"},
		{token.LT, "<"},
		{token.INT, "10"},
		{token.GT, ">"},
		{token.INT, "5"},
		{token.SEMICOLON, ";"},

		// if (5 < 10) { return true; } else { return false; }
		{token.IF, "if"},
		{token.LPAREN, "("},
		{token.INT, "5"},
		{token.LT, "<"},
		{token.INT, "10"},
		{token.RPAREN, ")"},
		{token.LBRACE, "{"},
		{token.RETURN, "return"},
		{token.TRUE, "true"},
		{token.SEMICOLON, ";"},
		{token.RBRACE, "}"},
		{token.ELSE, "else"},
		{token.LBRACE, "{"},
		{token.RETURN, "return"},
		{token.FALSE, "false"},
		{token.SEMICOLON, ";"},
		{token.RBRACE, "}"},

		// 10 == 10;
		{token.INT, "10"},
		{token.EQ, "=="},
		{token.INT, "10"},
		{token.SEMICOLON, ";"},

		// 10 != 9;
		{token.INT, "10"},
		{token.NOT_EQ, "!="},
		{token.INT, "9"},
		{token.SEMICOLON, ";"},

		// let just_go = "abcde";
		{token.LET, "let"},
		{token.IDENT, "just_go"},
		{token.ASSIGN, "="},
		{token.STRING, "abcde"},
		{token.SEMICOLON, ";"},

		// 最后 EOF
		{token.EOF, "\x00"},
	}

	fmt.Println("start test!!!")
	l := New(input)
	for i, tt := range test {
		tok := l.NextToken()
		if tok.Type != tt.expectedType {
			t.Fatalf("tests[%d] - tokentype wrong. expected=%q, got=%q, expected_litera=%q, go_litera=%q", i, tt.expectedType, tok.Type, tt.expectedLitera, tok.Literal)

		}

		if tok.Literal != tt.expectedLitera {
			t.Fatalf("tests[%d] - literal wrong. expected=%q, got=%q", i, tt.expectedLitera, tok.Literal)
		}
	}

	fmt.Println("end test!!!")
}
