package win

import (
	. "react/sim"
	. "react/sim/config"
	. "react/sim/structs"
)

// demand monitor is a demand estimator.  it monitors the demand from the past
// and estimates future demand based on a windowed history
type WindowEstimator struct {
	n uint64
	w *Window
}

var _ Estimator = new(WindowEstimator)
var _ Monitor = new(WindowEstimator)

func NewWindowEstimator(n uint64) *WindowEstimator {
	if n == 0 {
		panic("zero window length")
	}

	ret := new(WindowEstimator)
	ret.w = NewWindow()
	ret.n = n
	return ret
}

func (self *WindowEstimator) Tell(demand Matrix) {
	// println(demand.MatrixStr())
	demand = demand.Clone()

	if uint64(self.w.Len()) < self.n {
		self.w.Enq(demand)
	} else {
		self.w.Shift(demand)
	}
}

func (self *WindowEstimator) Estimate() (Matrix, uint64) {
	sum := self.w.Sum()
	hisLen := uint64(self.w.Len())

	if hisLen != WeekLen && hisLen > 0 {
		if hisLen >= WeekLen {
			panic("bug")
		}
		sum.Mul(WeekLen)
		sum.Div(hisLen)
	}

	// println(sum.MatrixStr(), WeekLen)
	return sum, WeekLen
}
