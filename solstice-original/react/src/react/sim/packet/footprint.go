package packet

import (
	"fmt"
	"react/sim/clock"
)

// Locations
const (
	Source = iota
	SwitchBuffer
	DownMerger
	Destination
	Dropped
)

func LocStr(loc int) string {
	switch loc {
	case Source:
		return "src"
	case SwitchBuffer:
		return "swbuf"
	case DownMerger:
		return "merge"
	case Destination:
		return "dest"
	case Dropped:
		return "drop"
	default:
		return fmt.Sprintf("loc?%d", loc)
	}
}

type Footprint struct {
	Location    int
	TimeReached uint64
}

func NewFootprint(location int) *Footprint {
	ret := new(Footprint)
	ret.Location = location
	ret.TimeReached = clock.T
	return ret
}

func (self *Footprint) String() string {
	return fmt.Sprintf("@%d fp=%s",
		self.TimeReached,
		LocStr(self.Location),
	)
}
