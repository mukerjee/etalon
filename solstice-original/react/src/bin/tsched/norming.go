package main

import (
	. "react/sim/structs"
)

func normDown(m Matrix, norm uint64) {
	sums := NewVector()
	m.NormDown(norm, sums)
}
