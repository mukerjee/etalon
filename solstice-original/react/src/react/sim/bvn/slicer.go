package bvn

import (
	. "react/sim/structs"
)

type Slicer interface {
	// returns number of ports mapped in pm
	Slice(m Sparse, pm PortMap) int

	// reset the slicer
	Reset()
}
