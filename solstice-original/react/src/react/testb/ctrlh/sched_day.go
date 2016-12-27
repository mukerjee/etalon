package ctrlh

import (
	"bytes"
	"fmt"

	"react/testb/setup"
)

type SchedDay struct {
	Index int
	Len   uint64
	Lanes []*SchedLane
}

func (self *SchedDay) WriteTo(buf *bytes.Buffer) {
	fmt.Fprintf(buf, "d%d %d:", self.Index, self.Len)
	for _, lane := range self.Lanes {
		fmt.Fprintf(buf, " %d-%d/%d", lane.Src, lane.Dest, lane.Rate)
	}
	fmt.Fprintln(buf)
}

func (self *SchedDay) Check() bool {
	srcPicked := make([]bool, setup.Nhost)
	destPicked := make([]bool, setup.Nhost)

	for _, lane := range self.Lanes {
		if srcPicked[lane.Src] {
			return false
		}
		if destPicked[lane.Dest] {
			return false
		}

		srcPicked[lane.Src] = true
		destPicked[lane.Dest] = true
	}

	return true
}
