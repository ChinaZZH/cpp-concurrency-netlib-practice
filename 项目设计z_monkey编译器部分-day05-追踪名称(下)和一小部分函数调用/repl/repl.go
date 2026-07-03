package repl

import (
	"bufio"
	"fmt"
	"io"
	"monkey/code"
	"monkey/compiler"
	"monkey/lexer"
	"monkey/parser"
	"monkey/vm"
)

const PROMPT = ">> "

func Start(in io.Reader, out io.Writer) {
	scanner := bufio.NewScanner(in)

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

		comp := compiler.New()
		err := comp.Compile(program)
		if err != nil {
			fmt.Println(out, "Compiler error:", err)
			continue
		}

		if compileMode {

			byteCode := comp.ByteCode()
			fmt.Fprintln(out, "=====Bytecode=======")
			fmt.Fprintln(out, code.Disassmble(byteCode))
			fmt.Fprintln(out, "=====Constants======")
			for i, c := range comp.Constants() {
				fmt.Fprintf(out, "%d: %v\n", i, c)
			}

			continue
		}

		vmInstance := vm.New(comp.ByteCode(), comp.Constants())
		err = vmInstance.Run()
		if err != nil {
			fmt.Fprintln(out, "VM error:", err)
			continue
		}

		// 获取栈顶结果
		if vmInstance.LastPoppedStackElem() != nil {
			fmt.Fprintln(out, vmInstance.LastPoppedStackElem().Inspect())
		}

	}
}
