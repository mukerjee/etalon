package main

import (
	"time"
)

type Counter struct {
	index  int
	source string

	sum     uint64
	i       int
	history []uint64
	n       uint64
	nvalid  int

	pipe chan uint64
}

func NewCounter(source string, index int) *Counter {
	ret := new(Counter)
	ret.index = index
	ret.source = source
	ret.history = make([]uint64, hisLen)
	ret.pipe = make(chan uint64)

	go ret.idleCount()
	go ret.serve()

	return ret
}

func (self *Counter) Count(n int) {
	self.pipe <- uint64(n)
}

func (self *Counter) idleCount() {
	for {
		self.pipe <- uint64(0)
		time.Sleep(time.Second / 5)
	}
}

func normalizeBandw(bw uint64) uint64 {
	return bw * uint64(time.Second) / uint64(sampleInterval)
}

func (self *Counter) serve() {
	ticks := time.Tick(sampleInterval)

	for n := range self.pipe {
		self.n += uint64(n)

		if len(ticks) > 0 {
			<-ticks

			self.sum -= self.history[self.i]
			self.sum += self.n
			save := self.n
			self.history[self.i] = self.n
			self.n = 0

			self.i++
			if self.nvalid < hisLen {
				self.nvalid++
			}
			if self.i == hisLen {
				self.i = 0
			}

			avg := self.sum / uint64(self.nvalid)

			statReport <- NewStat(self.source, self.index,
				normalizeBandw(save),
				normalizeBandw(avg))
		}
	}
}
