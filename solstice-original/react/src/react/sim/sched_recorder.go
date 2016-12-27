package sim

import (
	"fmt"
	"io"

	"react/sim/clock"
	. "react/sim/config"
	. "react/sim/structs"
)

type SchedRecorder struct {
	out io.Writer
}

func NewSchedRecorder(out io.Writer) *SchedRecorder {
	ret := new(SchedRecorder)
	ret.out = out
	fmt.Fprintf(out, "n=%d\n", Nhost)
	fmt.Fprintf(out, "linkbw=%d\n", LinkBw)
	fmt.Fprintf(out, "packbw=%d\n", PackBw)

	return ret
}

func (self *SchedRecorder) printDaySched(d *Day) {
	for i := 0; i < Nhost; i++ {
		for j := 0; j < Nhost; j++ {
			lane := i*Nhost + j
			s := d.Sched[lane]
			if s > 0 {
				r := s * 100 / LinkBw
				_, e := fmt.Fprintf(self.out, " %d-%d/%d", i, j, r)
				noError(e)
			}
		}
	}
}

func (self *SchedRecorder) Log(days []*Day) {
	_, e := fmt.Fprintln(self.out)
	noError(e)

	_, e = fmt.Fprintf(self.out, "// T=%d\n", clock.T)
	noError(e)

	for i, d := range days {
		_, e = fmt.Fprintf(self.out, "d%d %d:", i, d.Len)
		noError(e)

		self.printDaySched(d)
		_, e = fmt.Fprintln(self.out)
		noError(e)
	}
}

var logExample = `
n=8

// T=0
d0 300: 0-1@90 1-2@90 2-3@90 3-4@90 4-5@90 5-6@90 6-7@90 7-0@90
...

`
