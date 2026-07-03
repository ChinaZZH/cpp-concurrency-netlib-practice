package vm

import "monkey/object"

// Frame 表示一个函数调用帧
type Frame struct {
	fn     *object.Closure // 指向被调用的闭包
	ip     int             // 指令指针（当前执行位置）
	locals []object.Object //局部变量
}

func NewFrame(fn *object.Closure, numLocals int) *Frame {
	return &Frame{
		fn:     fn,
		ip:     0,
		locals: make([]object.Object, numLocals),
	}
}
