package bvn

import (
	. "react/sim/structs"
)

// A filler fills a matrix into a doubly stochastic matrix
type Stuffer interface {
	Stuff(m Matrix, norm uint64) uint64
}

func GridStuff(s Stuffer, m Matrix, norm, grid uint64) uint64 {
	assert(norm%grid == 0)

	m.Div(grid)
	ret := s.Stuff(m, norm/grid)
	m.Mul(grid)

	return ret * grid
}
