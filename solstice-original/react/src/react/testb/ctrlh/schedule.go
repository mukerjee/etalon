package ctrlh

import (
	"bytes"
	"fmt"

	"react/testb/setup"
)

type Schedule struct {
	Nhost  int
	LinkBw uint64
	PackBw uint64

	Weeks []*SchedWeek
}

func newSchedule() *Schedule {
	ret := new(Schedule)
	ret.Nhost = setup.Nhost
	ret.LinkBw = setup.LinkBw
	ret.PackBw = setup.PackBw
	ret.Weeks = make([]*SchedWeek, 0, 1000)

	return ret
}

func (s *Schedule) WriteTo(buf *bytes.Buffer) {
	fmt.Fprintf(buf, "n=%d\n", s.Nhost)
	fmt.Fprintf(buf, "linkbw=%d\n", s.LinkBw)
	fmt.Fprintf(buf, "packbw=%d\n", s.PackBw)

	n := uint64(0)
	for _, week := range s.Weeks {
		fmt.Fprintln(buf)
		fmt.Fprintf(buf, "// T=%d\n", n)

		week.WriteTo(buf)
		n += week.Len()
	}
}

func (s *Schedule) String() string {
	buf := new(bytes.Buffer)
	s.WriteTo(buf)
	return buf.String()
}
