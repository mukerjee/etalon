package bvn

import (
	. "react/sim/config"
	. "react/sim/structs"
)

type SkewedStuffer struct {
	rowBuf, colBuf Vector
}

var _ Stuffer = new(SkewedStuffer)

func NewSkewedStuffer() *SkewedStuffer {
	ret := new(SkewedStuffer)
	ret.rowBuf = NewVector()
	ret.colBuf = NewVector()

	return ret
}

func (self *SkewedStuffer) Stuff(m Matrix, norm uint64) uint64 {
	rowBuf := self.rowBuf
	colBuf := self.colBuf

	sum := m.Sums(rowBuf, colBuf)
	if norm < sum {
		norm = sum
	}

	for {
		maxReachLane := -1
		maxReach := uint64(0)
		for i := 0; i < Nlane; i++ {
			row := i / Nhost
			col := i % Nhost
			rowDelta := norm - rowBuf[row]
			colDelta := norm - colBuf[col]
			delta := rowDelta
			if colDelta < delta {
				delta = colDelta
			}

			if delta > 0 {
				reach := m[i] + delta
				if reach > maxReach || maxReachLane < 0 {
					maxReach = reach
					maxReachLane = i
				}
			}
		}

		if maxReachLane < 0 {
			break
		}

		delta := maxReach - m[maxReachLane]
		m[maxReachLane] = maxReach
		rowBuf[maxReachLane/Nhost] += delta
		colBuf[maxReachLane%Nhost] += delta
	}

	return norm
}
