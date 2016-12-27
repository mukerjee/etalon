package bvn

import (
	"math/rand"

	. "react/sim/config"
	. "react/sim/structs"
)

type QuickStuffer struct {
	rowBuf, colBuf Vector
	rand           bool
}

var _ Stuffer = new(QuickStuffer)

func NewQuickStuffer() *QuickStuffer {
	ret := new(QuickStuffer)
	ret.rowBuf = NewVector()
	ret.colBuf = NewVector()

	return ret
}

func NewRandStuffer() *QuickStuffer {
	ret := NewQuickStuffer()
	ret.rand = true
	return ret
}

func quickStuff(m Matrix, rowBuf, colBuf Vector,
	n uint64, useRandom bool, stuffZero bool) uint64 {
	maxSum := m.Sums(rowBuf, colBuf)
	if n < maxSum {
		n = maxSum
	}
	if n == 0 {
		return 0
	}

	rowBuf.SubBy(n)
	colBuf.SubBy(n)

	var order []int
	if useRandom {
		order = rand.Perm(Nlane)
	}

	for i := 0; i < Nlane; i++ {
		index := i
		if order != nil {
			index = order[i]
		}

		row := index / Nhost
		col := index % Nhost
		rowDelta := rowBuf[row]
		colDelta := colBuf[col]
		if m[index] == 0 && !stuffZero {
			continue
		}
		if rowDelta > 0 && colDelta > 0 {
			delta := rowDelta
			if colDelta < delta {
				delta = colDelta
			}

			m[index] += delta
			rowBuf[row] -= delta
			colBuf[col] -= delta
		}
	}

	return n
}

func (self *QuickStuffer) Stuff(m Matrix, norm uint64) uint64 {
	quickStuff(m, self.rowBuf, self.colBuf, norm, self.rand, false)
	return quickStuff(m, self.rowBuf, self.colBuf, norm, self.rand, true)
}
