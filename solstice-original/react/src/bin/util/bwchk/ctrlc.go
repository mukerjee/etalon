package main

import (
	"os"
	"os/signal"
)

type CtrlC struct {
	got bool
	c   chan os.Signal
}

func newCtrlC() *CtrlC {
	ret := new(CtrlC)
	ret.c = make(chan os.Signal, 10)
	signal.Notify(ret.c, os.Interrupt)

	return ret
}

func (self *CtrlC) received() bool {
	for len(self.c) > 0 {
		<-self.c
		self.got = true
	}

	return self.got
}

var ctrlc = newCtrlC()

func running() bool {
	return !ctrlc.received()
}
