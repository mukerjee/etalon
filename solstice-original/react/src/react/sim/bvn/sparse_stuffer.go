package bvn

import (
	. "react/sim/structs"
)

type SparseStuffer interface {
	StuffSparse(sparse Sparse, norm uint64) uint64
}

func StuffWithSparse(stuffer SparseStuffer, m Matrix, norm uint64) uint64 {
	sp := m.Sparse()
	ret := stuffer.StuffSparse(sp, norm)
	copy(m, sp.Matrix())
	return ret
}
