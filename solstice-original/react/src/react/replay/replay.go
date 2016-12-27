package replay

import (
	"io"
	"os"
)

type Replay struct {
	Name   string
	Memo   string
	LinkBw uint64
	PackBw uint64

	chanNames []string
	ticks     []*Tick
	nhost     int
	zeros     Matrix
}

func NewReplay(nhost int) *Replay {
	if nhost <= 1 {
		panic("invalid nhost")
	}

	ret := new(Replay)
	ret.nhost = nhost
	ret.chanNames = make([]string, 0, 10)
	ret.ticks = make([]*Tick, 0, 1e5)
	ret.zeros = NewMatrix(nhost)

	return ret
}

func (self *Replay) QueryChan(name string) int {
	for i, c := range self.chanNames {
		if c == name {
			return i
		}
	}
	return -1
}

func (self *Replay) Matrix(c int, t uint64) Matrix {
	if t < uint64(len(self.ticks)) {
		return self.ticks[int(t)].chans[c]
	}
	return self.zeros
}

func (self *Replay) Tags(t uint64) []string {
	return self.ticks[int(t)].tags
}

func (self *Replay) addChan(c string) bool {
	for _, name := range self.chanNames {
		if name == c {
			return false
		}
	}
	self.chanNames = append(self.chanNames, c)
	return true
}

func (self *Replay) Len() int {
	return len(self.ticks)
}

func (self *Replay) addTick(t *Tick) uint64 {
	self.ticks = append(self.ticks, t)
	return uint64(len(self.ticks))
}

func (self *Replay) Nhost() int { return self.nhost }

func (self *Replay) PrintTo(w io.Writer) error {
	header := &Header{
		Name:      self.Name,
		Memo:      self.Memo,
		LinkBw:    self.LinkBw,
		PackBw:    self.PackBw,
		Nhost:     self.nhost,
		ChanNames: self.chanNames,
	}

	rec, e := NewRecorder(w, header)
	if e != nil {
		return e
	}

	for _, tick := range self.ticks {
		e = rec.Record(tick)
		if e != nil {
			return e
		}
	}

	return nil
}

func (self *Replay) Save(path string) error {
	f, e := os.Create(path)
	if e != nil {
		return e
	}
	defer f.Close()

	e = self.PrintTo(f)
	if e != nil {
		return e
	}

	return nil
}
