package sim

import (
	. "react/sim/structs"
)

type Scheduler interface {
	// Based on the "demand" given for the next "wind" amount of ticks
	// Returns a series of days, and the allocated bandwidth matrix
	Schedule(leftover, demand Matrix, wind uint64) (days []*Day, bandw Matrix)
}
