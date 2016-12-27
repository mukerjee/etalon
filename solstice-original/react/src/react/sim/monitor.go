package sim

import (
	"react/sim/structs"
)

type Monitor interface {
	// Tells the demand for the current tick.
	Tell(demand structs.Matrix)
}
