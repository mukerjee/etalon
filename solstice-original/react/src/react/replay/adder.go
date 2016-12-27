package replay

type Adder struct {
	Chan   int
	Start  uint64
	Window uint64
}

func (self *Adder) Add(rep *Replay) Matrix {
	ret := NewMatrix(rep.Nhost())

	for i := uint64(0); i < self.Window; i++ {
		m := rep.Matrix(self.Chan, self.Start+i)
		ret.Madd(m)
	}

	self.Start += self.Window

	return ret
}
