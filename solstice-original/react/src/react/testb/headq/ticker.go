package headq

import (
	"react/replay"
)

// A Ticker merges several dump tickers, emits replay ticks
type Ticker struct {
	t     uint64
	chans []*DumpChan
	tick  *replay.Tick
}

func NewTicker(dumpers ...Dumper) *Ticker {
	ret := new(Ticker)
	ret.t = 0
	ret.chans = make([]*DumpChan, len(dumpers))
	for i, d := range dumpers {
		ret.chans[i] = NewDumpChan(d)
	}

	return ret
}

func (self *Ticker) Check() bool {
	if self.tick == nil {
		self.tick = replay.NewTick(len(self.chans))
	} else {
		self.tick.Clear()
	}

	allClosed := true
	for _, c := range self.chans {
		self.tick.Chan(c.Tick(self.t))
		allClosed = allClosed && c.Closed()
	}

	self.t++
	return !allClosed
}

func (self *Ticker) Tick() *replay.Tick {
	return self.tick
}
