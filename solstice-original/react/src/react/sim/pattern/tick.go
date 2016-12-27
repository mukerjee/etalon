package pattern

import (
	. "react/sim/config"
	. "react/sim/structs"
)

type Tick struct {
	D    Matrix
	Rsum Vector
	Csum Vector
}

func NewTick() *Tick {
	ret := new(Tick)
	ret.D = NewMatrix()
	ret.Rsum = NewVector()
	ret.Csum = NewVector()
	return ret
}

func (t *Tick) Add(row, col int, a uint64) {
	lane := row*Nhost + col
	t.D[lane] += a
	t.Rsum[row] += a
	t.Csum[col] += a
}

func (t *Tick) Madd(m Matrix) {
	t.D.Madd(m)
	t.D.RowSum(t.Rsum)
	t.D.ColSum(t.Csum)
}

func (t *Tick) ToAdd(row, col int, bandw, max uint64) uint64 {
	rspace := bandw - t.Rsum[row]
	cspace := bandw - t.Csum[row]

	r := rspace
	if r > rspace {
		r = cspace
	}
	if max > 0 && r > max {
		r = max
	}
	return r
}
