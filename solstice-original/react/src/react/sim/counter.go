package sim

import (
	. "react/sim/packet"
	. "react/sim/structs"
)

type Counter struct {
	Cur Matrix
	Sum Matrix
}

func NewCounter() *Counter {
	ret := new(Counter)
	ret.Cur = NewMatrix()
	ret.Sum = NewMatrix()
	return ret
}

func (self *Counter) Count(lane int, n uint64) {
	self.Cur[lane] += n
	self.Sum[lane] += n
}

func (self *Counter) CountPacket(p *Packet) {
	self.Count(p.Lane, p.Size)
}

func (self *Counter) CountMatrix(m Matrix) {
	if m == nil {
		return
	}

	self.Cur.Madd(m)
	self.Sum.Madd(m)
}

func (self *Counter) ClearSum() {
	self.Sum.Clear()
}

func (self *Counter) Clear() {
	self.Cur.Clear()
}

func (self *Counter) CurTotal() uint64 {
	return self.Cur.Sum()
}
