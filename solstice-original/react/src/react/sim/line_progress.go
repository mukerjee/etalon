package sim

import (
	"fmt"
	"os"
	"time"
)

type Flows interface {
	Nflow() int
}

type LineProgress struct {
	NoSingleLine bool
	ticker       <-chan time.Time

	Flows Flows
}

func NewLineProgress() *LineProgress {
	ret := new(LineProgress)
	ret.ticker = time.Tick(time.Second / 10)
	return ret
}

var _ Progress = new(LineProgress)

const csi = "\x1b["

func (self *LineProgress) lineRevert() {
	fmt.Fprint(os.Stderr, csi, "A", csi, 0, "G", csi, "K")
}

func ratio(a, b uint64) float64 {
	if a == 0 && b == 0 {
		return 0
	}
	if b == 0 && a > 0 {
		return -1
	}
	return float64(a) / float64(b) * 100
}

func numStr(i uint64) string {
	scales := []struct {
		unit uint64
		name string
	}{
		{uint64(1), ""},
		{uint64(1e3), "k"},
		{uint64(1e6), "m"},
		{uint64(1e9), "g"},
		{uint64(1e12), "t"},
		{uint64(1e15), "p"},
	}

	for _, s := range scales {
		u := s.unit
		if u == 1 {
			if i < 10000 {
				return fmt.Sprintf("%d", i)
			}
		}

		if i/u < 1000 {
			return fmt.Sprintf("%d%s", (i+u/2)/u, s.name)
		}
	}

	return "BIG!"
}

func (self *LineProgress) printTick(s *ProgressStat) {
	if s.Packput+s.Circput != s.Goodput {
		fmt.Fprintf(os.Stderr, "!! ")
	}

	line := fmt.Sprintf("t=%d circ=%s(%.1f%%) pack=%s(%.1f%%) "+
		"good=%s(%.1f%%) drop=%s(%.1f%%) cap=%s",
		s.T,
		numStr(s.Circput), ratio(s.Circput, s.Capacity),
		numStr(s.Packput), ratio(s.Packput, s.Capacity),
		numStr(s.Goodput), ratio(s.Goodput, s.Capacity),
		numStr(s.Dropped), ratio(s.Dropped, s.Capacity),
		numStr(s.Capacity),
	)

	if self.Flows != nil {
		nflow := self.Flows.Nflow()
		line += fmt.Sprintf(" nflow=%d", nflow)
	}

	fmt.Fprintln(os.Stderr, line)
}

func (self *LineProgress) Start(s *ProgressStat) {
	self.printTick(s)
}

func (self *LineProgress) Tick(s *ProgressStat) {
	if len(self.ticker) > 0 {
		<-self.ticker
		self.lineRevert()
		self.printTick(s)
	}
}

func (self *LineProgress) Stop(s *ProgressStat) {
	self.lineRevert()
	self.printTick(s)
}
