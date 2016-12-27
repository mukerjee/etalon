package main

import (
	"fmt"
	"os"
	"time"
)

type Reporter struct {
	stats     []*Stat
	needClear bool
	pipe      chan *Stat
}

const CSI = "\x1b["

func NewReporter(nslot int, closed <-chan os.Signal) *Reporter {
	ret := new(Reporter)
	ret.stats = make([]*Stat, nslot)
	ret.pipe = make(chan *Stat, 3000)

	go ret.serve(closed)

	return ret
}

func (self *Reporter) Update(s *Stat) {
	self.pipe <- s
}

func (self *Reporter) serve(closed <-chan os.Signal) {
	ticker := time.NewTicker(time.Second / 2)

	for len(closed) == 0 {
		if len(self.pipe) > 0 {
			for len(self.pipe) > 0 {
				s := <-self.pipe
				self.stats[s.index] = s
			}
		} else {
			time.Sleep(time.Second / 10)

			for _, s := range self.stats {
				if s == nil {
					continue
				}
				if s.ttl > 0 {
					s.ttl--
				}
			}
		}

		if len(ticker.C) > 0 {
			<-ticker.C
			self.Print()
		}
	}
}

func (self *Reporter) Print() {
	if self.needClear {
		fmt.Print(CSI, len(self.stats), "A")
	}

	for _, s := range self.stats {
		if s == nil || s.ttl == 0 {
			fmt.Print("-")
		} else {
			fmt.Print(s)
		}

		fmt.Print(CSI, "K") // clear line
		fmt.Println()
	}

	self.needClear = true
}
