package main

import (
	"math/rand"

	. "react/sim/structs"
)

type DemandMaker interface {
	Make(norm uint64, r *rand.Rand) Matrix
}
