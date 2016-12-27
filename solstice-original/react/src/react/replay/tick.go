package replay

import (
	"bytes"
	"fmt"
	"io"
	"strings"
)

type Tick struct {
	tags  []string
	chans []Matrix
}

func NewTick(nchan int) *Tick {
	ret := new(Tick)
	ret.tags = make([]string, 0, 10)
	ret.chans = make([]Matrix, 0, nchan)

	return ret
}

func (self *Tick) Clear() {
	self.tags = self.tags[0:0]
	self.chans = self.chans[0:0]
}

func (self *Tick) Tag(t string) {
	self.tags = append(self.tags, t)
}

func (self *Tick) Chan(m Matrix) int {
	self.chans = append(self.chans, m)
	return len(self.chans)
}

func (self *Tick) Nchan() int {
	return len(self.chans)
}

func (self *Tick) PrintTo(w io.Writer, t uint64,
	chanNames []string, nhost int) error {
	buf := new(bytes.Buffer)

	if len(chanNames) != len(self.chans) {
		panic("chan mismatch")
	}

	fmt.Fprintf(buf, "t=%d", t)
	if len(self.tags) > 0 {
		fmt.Fprintf(buf, " // %s", strings.Join(self.tags, ","))
	}
	fmt.Fprintln(buf)

	for i, name := range chanNames {
		fmt.Fprintf(buf, "%s:", name)
		m := self.chans[i]

		for r := 0; r < nhost; r++ {
			if r > 0 {
				fmt.Fprint(buf, ";")
			}
			base := r * nhost
			for c := 0; c < nhost; c++ {
				n := m[base+c]
				if n == 0 {
					fmt.Fprint(buf, " -")
				} else {
					fmt.Fprintf(buf, " %d", n)
				}
			}
		}

		fmt.Fprintln(buf)
	}

	_, e := buf.WriteTo(w)

	return e
}
