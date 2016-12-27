package win

import (
	"container/list"
	. "react/sim/structs"
)

type Window struct {
	w           *list.List
	sum         Matrix
	maintainSum bool
}

func NewWindow() *Window {
	ret := new(Window)
	ret.w = list.New()
	ret.sum = NewMatrix()
	return ret
}

func (self *Window) SetMaintainSum(f bool) {
	if f && !self.maintainSum {
		self.Sum()
	}
	self.maintainSum = f
}

func (self *Window) Len() int {
	return self.w.Len()
}

func (self *Window) Clear() {
	self.sum.Clear()
	self.w = list.New()
}

func (self *Window) Enq(m Matrix) {
	self.w.PushBack(m)

	if self.maintainSum {
		self.sum.Madd(m)
	}
}

func (self *Window) Deq() Matrix {
	front := self.w.Front()
	if front == nil {
		return nil
	}

	e := self.w.Remove(front)
	ret := e.(Matrix)

	if self.maintainSum {
		self.sum.Msub(ret)
	}

	return ret
}

func (self *Window) Front() Matrix {
	front := self.w.Front()
	if front == nil {
		return nil
	}

	return front.Value.(Matrix)
}

func (self *Window) Shift(m Matrix) Matrix {
	self.Enq(m)
	return self.Deq()
}

func (self *Window) Sum() Matrix {
	if !self.maintainSum {
		// needs update
		self.sum.Clear()

		for e := self.w.Front(); e != nil; e = e.Next() {
			self.sum.Madd(e.Value.(Matrix))
		}
	}

	return self.sum
}
