package sim

import (
	"react/sim/structs"
)

type Estimator interface {
	// Returns the demand for the next weeklen
	Estimate() (structs.Matrix, uint64)
}
