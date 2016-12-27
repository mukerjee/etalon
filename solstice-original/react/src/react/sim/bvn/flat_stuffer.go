package bvn

import (
	. "react/sim/config"
	. "react/sim/structs"
)

type FlatStuffer struct {
	rowBuf, colBuf Vector
}

var _ Stuffer = new(FlatStuffer)

func NewFlatStuffer() *FlatStuffer {
	ret := new(FlatStuffer)
	ret.rowBuf = NewVector()
	ret.colBuf = NewVector()

	return ret
}

func (self *FlatStuffer) Stuff(m Matrix, norm uint64) uint64 {
	rowBuf := self.rowBuf
	colBuf := self.colBuf

	maxSum := m.Sums(rowBuf, colBuf)
	if norm < maxSum {
		norm = maxSum
	}
	if norm == 0 {
		return 0
	}

	rowBuf.SubBy(norm)
	colBuf.SubBy(norm)

	total := m.Sum()
	div := norm*uint64(Nhost) - total

	if div > 0 {
		for i := 0; i < Nhost; i++ {
			base := i * Nhost
			for j := 0; j < Nhost; j++ {
				m[base+j] += rowBuf[i] * colBuf[j] / div
			}
		}
	}

	return quickStuff(m, self.rowBuf, self.colBuf, norm, false, true)
}
