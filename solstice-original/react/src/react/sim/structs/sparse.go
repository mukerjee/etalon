package structs

import (
	"math"

	. "react/sim/config"
)

type Sparse []SparseCol
type SparseCol map[int]uint64

func NewSparse() Sparse {
	return make([]SparseCol, Nhost)
}

func RowCol(lane int) (int, int) { return lane / Nhost, lane % Nhost }
func Lane(row, col int) int      { return row*Nhost + col }

func (s Sparse) Get(lane int) uint64 {
	row, col := RowCol(lane)
	return s.At(row, col)
}

func (s Sparse) At(row, col int) uint64 {
	scol := s[col]
	if scol == nil {
		return 0
	}
	return scol[row]
}

func (s Sparse) SetAt(row, col int, a uint64) {
	if s[col] == nil {
		s[col] = make(SparseCol)
	}
	scol := s[col]
	if a == 0 {
		if _, found := scol[row]; found {
			delete(scol, row)
		}
	} else {
		scol[row] = a
	}
}

func (s Sparse) Set(lane int, a uint64) {
	row, col := RowCol(lane)
	s.SetAt(row, col, a)
}

func (s Sparse) SubTrim(row, col int, delta, min uint64) {
	if delta == 0 {
		panic("bug")
	}

	scol := s[col]
	if scol == nil {
		return
	}
	n, found := scol[row]
	if !found {
		return
	}
	if n <= delta || n-delta < min {
		delete(scol, row)
		return
	}
	scol[row] = n - delta
}

func (s Sparse) Sub(row, col int, delta uint64) {
	s.SubTrim(row, col, delta, 0)
}

func (s Sparse) AddAt(row, col int, a uint64) {
	s.SetAt(row, col, s.At(row, col)+a)
}

func (m Matrix) Sparse() Sparse {
	ret := NewSparse()

	for i, d := range m {
		if d > 0 {
			ret.Set(i, d)
		}
	}

	return ret
}

func (s Sparse) Matrix() Matrix {
	ret := NewMatrix()

	for col, scol := range s {
		for row, n := range scol {
			ret[Lane(row, col)] = n
		}
	}

	return ret
}

func (s Sparse) Assign(m Matrix) {
	s.Clear()
	for lane, n := range m {
		if n > 0 {
			s.Set(lane, n)
		}
	}
}

func (s Sparse) Clear() {
	for i := range s {
		s[i] = nil
	}
}

func (s Sparse) MinInMap(pm PortMap) uint64 {
	ret := uint64(math.MaxUint64)
	nvalid := 0

	for row, col := range pm {
		if col < 0 {
			continue
		}
		nvalid++
		n := s.At(row, col)
		if n < ret {
			ret = n
		}
	}

	if nvalid == 0 {
		return 0
	}
	return ret
}

func (s Sparse) SubInMap(pm PortMap, w uint64) {
	for src, dest := range pm {
		if dest < 0 {
			continue
		}
		s.Sub(src, dest, w)
	}
}

func (s Sparse) ThresFrom(other Sparse, min uint64) Sparse {
	s.Clear()
	for col, scol := range other {
		for row, n := range scol {
			if n >= min {
				s.SetAt(row, col, n)
			}
		}
	}
	return s
}

func (s Sparse) Thres(min uint64) Sparse {
	for col, scol := range s {
		for row, n := range scol {
			if n < min {
				s.SetAt(row, col, 0)
			}
		}
	}
	return s
}

func (s Sparse) CopyFrom(other Sparse) Sparse {
	s.Clear()
	for col, scol := range other {
		for row, n := range scol {
			s.SetAt(row, col, n)
		}
	}

	return s
}

func (s Sparse) Sums(rowSum Vector, colSum Vector) uint64 {
	if rowSum == nil {
		rowSum = NewVector()
	}
	if colSum == nil {
		colSum = NewVector()
	}
	rowSum.Clear()
	colSum.Clear()
	maxSum := uint64(0)

	for col, scol := range s {
		for row, n := range scol {
			rowSum[row] += n
			colSum[col] += n
			if rowSum[row] > maxSum {
				maxSum = rowSum[row]
			}
			if colSum[col] > maxSum {
				maxSum = colSum[col]
			}
		}
	}

	return maxSum
}
