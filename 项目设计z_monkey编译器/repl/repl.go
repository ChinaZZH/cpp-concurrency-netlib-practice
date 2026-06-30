package repl

import (
	"bufio"
	"fmt"
	"io"
	"monkey/code"
	"monkey/compiler"
	"monkey/evaluator"
	"monkey/lexer"
	"monkey/object"
	"monkey/parser"
)

const PROMPT = ">> "

func Start(in io.Reader, out io.Writer) {
	scanner := bufio.NewScanner(in)
	env := object.NewEnvironment()

	//constants := []interface{}{}
	//globals := make([]object.Object, vm.GlobalsSize)

	for {
		fmt.Fprint(out, PROMPT)
		scanned := scanner.Scan()
		if !scanned {
			return
		}

		line := scanner.Text()
		if line == "exit" || line == "quit" {
			return
		}

		// 编译模式: 输入前缀是 ":c" 或 ":compile"
		compileMode := false
		if len(line) > 2 && (line[:2] == ":c" || line[:2] == ":C") {
			compileMode = true
			line = line[3:] // 去掉 ":c " 前缀
			if line == "" {
				fmt.Println(out, "usage: c <expression>")
				continue
			}
		}

		l := lexer.New(line)
		p := parser.New(l.Tokens())
		program := p.ParseProgram()

		if len(p.Errors()) != 0 {
			for _, msg := range p.Errors() {
				fmt.Fprintln(out, msg)
			}

			continue
		}

		if !compileMode {
			evaluated := evaluator.Eval(program, env)
			if evaluated != nil {
				fmt.Fprintln(out, evaluated.Inspect())
			}
		} else {
			comp := compiler.New()
			err := comp.Compile(program)
			if err != nil {
				fmt.Println(out, "Compiler error:", err)
				continue
			}

			byteCode := comp.ByteCode()
			fmt.Fprintln(out, "=====Bytecode=======")
			fmt.Fprintln(out, code.Disassmble(byteCode))
			fmt.Fprintln(out, "=====Constants======")
			for i, c := range comp.Constants() {
				fmt.Fprintf(out, "%d: %v\n", i, c)
			}
		}

	}
}
