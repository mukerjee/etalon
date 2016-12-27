package win

import (
	"react/sim/config"
	"react/sim/density"
	"react/sim/netlog"
	"react/sim/simlog"
	"react/sim/structs"
)

type WindowTracker struct {
	i uint64
	n uint64
	w *Window
	d *density.Density
	D *density.Density

	Logger *simlog.MatWriter
}

func NewWindowTracker(n uint64) *WindowTracker {
	if n == 0 {
		panic("zero window length")
	}

	ret := new(WindowTracker)
	ret.n = n
	ret.w = NewWindow()
	ret.d = new(density.Density)
	ret.D = new(density.Density)

	return ret
}

func (self *WindowTracker) Tell(demand structs.Matrix) {
	// netlog.Println(demand)
	demand = demand.Clone()

	if uint64(self.w.Len()) < self.n {
		self.w.Enq(demand)
	} else {
		self.w.Shift(demand)
	}

	self.i++
	if self.i < self.n {
		return
	}

	self.track()
	self.i = 0
}

func (self *WindowTracker) track() {
	m := self.w.Sum()
	if self.Logger != nil {
		self.Logger.Write(m)
	}

	self.d.Count(m)
	self.D.Max(self.d)

	// netlog.Println(m.MatrixStr())
	netlog.Print("\n", self.d.String())
}

func (self *WindowTracker) Estimate() (structs.Matrix, uint64) {
	sum := self.w.Sum()
	hisLen := uint64(self.w.Len())

	if hisLen != config.WeekLen && hisLen > 0 {
		if hisLen >= config.WeekLen {
			panic("bug")
		}
		sum.Mul(config.WeekLen)
		sum.Div(hisLen)
	}

	return sum, config.WeekLen
}
