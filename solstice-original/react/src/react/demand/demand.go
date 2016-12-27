package demand

import (
	"bytes"
	"fmt"
	"io"
	"os"
)

type Demand struct {
	Name string
	Memo string

	periods []*Period
	n       uint64
	nhost   int
}

func NewDemand(nhost int) *Demand {
	ret := new(Demand)
	ret.nhost = nhost
	ret.periods = make([]*Period, 0, 1000)

	return ret
}

func (self *Demand) Nhost() int { return self.nhost }

func (self *Demand) PrintTo(w io.Writer) error {
	if self == nil {
		panic("bug")
	}

	header := &header{
		Name:  self.Name,
		Memo:  self.Memo,
		Nhost: self.nhost,
	}

	e := header.PrintTo(w)
	if e != nil {
		return e
	}

	t := uint64(0)

	for _, p := range self.periods {
		_, e = fmt.Fprintln(w)
		if e != nil {
			return e
		}

		_, e = fmt.Fprintf(w, "%% T=%d\n", t)
		if e != nil {
			return e
		}

		e = p.PrintTo(w)
		if e != nil {
			return e
		}

		t += p.N
	}

	return nil
}

func (self *Demand) Save(path string) error {
	fout, e := os.Create(path)
	if e != nil {
		return e
	}
	defer func() {
		e := fout.Close()
		if e != nil {
			panic(e)
		}
	}()
	return self.PrintTo(fout)
}

func (self *Demand) Add(p *Period) {
	if p.N == 0 {
		panic("period of zero length")
	}
	self.periods = append(self.periods, p)
	self.n += p.N
}

func (self *Demand) Period(i int) *Period {
	return self.periods[i]
}

func (self *Demand) Len() uint64 {
	return self.n
}

func (self *Demand) Nperiod() int {
	return len(self.periods)
}

func (self *Demand) String() string {
	buf := new(bytes.Buffer)
	e := self.PrintTo(buf)
	if e != nil {
		panic(e)
	}

	return buf.String()
}
