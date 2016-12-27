package sim

import (
	"io"

	"react/replay"
	. "react/sim/config"
	. "react/sim/structs"
)

type Recorder struct {
	rec  *replay.Recorder
	tick *replay.Tick
}

func NewRecorder(w io.Writer) *Recorder {
	ret := new(Recorder)

	header := &replay.Header{
		Nhost:     Nhost,
		LinkBw:    LinkBw,
		PackBw:    PackBw,
		ChanNames: chanNames,
	}

	var e error
	ret.rec, e = replay.NewRecorder(w, header)
	noError(e)

	ret.tick = replay.NewTick(len(chanNames))

	return ret
}

var chanNames = []string{
	"send",
	"recv",
	"circ",
	"pack",
	"drop",
	"sched",
}

type Tick struct {
	T uint64

	NewWeek bool
	Dusk    bool
	Dawn    bool

	Send  Matrix
	Recv  Matrix
	Circ  Matrix
	Pack  Matrix
	Drop  Matrix
	Sched Matrix
}

func (self *Recorder) _chan(m Matrix) {
	self.tick.Chan(replay.Matrix(m))
}

func (self *Recorder) Record(t *Tick) {
	tick := self.tick
	tick.Clear()

	if t.NewWeek {
		tick.Tag("new-week")
	}
	if t.Dusk {
		tick.Tag("dusk")
	}
	if t.Dawn {
		tick.Tag("dawn")
	}
	self._chan(t.Send)
	self._chan(t.Recv)
	self._chan(t.Circ)
	self._chan(t.Pack)
	self._chan(t.Drop)
	self._chan(t.Sched)

	if tick.Nchan() != len(chanNames) {
		panic("bug")
	}

	self.rec.Record(tick)
}
