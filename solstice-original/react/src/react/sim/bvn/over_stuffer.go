package bvn

import (
	. "react/sim/config"
	. "react/sim/structs"
)

type OverStuffer struct {
	rowSum, colSum Vector
}

func NewOverStuffer() *OverStuffer {
	ret := new(OverStuffer)
	ret.rowSum = NewVector()
	ret.colSum = NewVector()

	return ret
}

var _ Stuffer = new(OverStuffer)
var _ SparseStuffer = new(OverStuffer)

func (self *OverStuffer) Stuff(m Matrix, norm uint64) uint64 {
	return StuffWithSparse(self, m, norm)
}

func (self *OverStuffer) StuffSparse(sparse Sparse, norm uint64) uint64 {
	rowSum := self.rowSum
	colSum := self.colSum

	maxSum := sparse.Sums(rowSum, colSum)
	if norm < maxSum {
		norm = maxSum
	}

	for row := 0; row < Nhost; row++ {
		for col := 0; col < Nhost; col++ {
			rowDelta := norm - rowSum[row]
			colDelta := norm - colSum[col]
			delta := rowDelta
			if delta > colDelta {
				delta = colDelta
			}

			if delta == 0 {
				continue
			}

			n := sparse.At(row, col)
			assert(n+delta <= norm)
			sparse.SetAt(row, col, n+delta)
		}
	}

	return maxSum
}
