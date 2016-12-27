package main

import (
	"bytes"
	"fmt"
	"time"
)

type BriefHeader struct {
	Capacity uint64
	Demand   uint64
	Seed     int64
}

func (self *BriefHeader) String() string {
	return fmt.Sprintf("# %d %d %d",
		self.Demand, self.Capacity, self.Seed)
}

type Brief struct {
	Header    *BriefHeader
	SchedName string

	Nday int

	Circ       uint64
	CircQueued uint64
	Pack       uint64

	TimeUsed time.Duration
}

func (self *Brief) perc(a uint64) string {
	return fmt.Sprintf("%d, %.2f%% cap, %.2f%% dem",
		a,
		float64(a)/float64(self.Header.Capacity)*100,
		float64(a)/float64(self.Header.Demand)*100,
	)
}

func (self *Brief) String() string {
	return fmt.Sprintf("%s %d %d %d %d %v",
		self.SchedName, self.Nday,
		self.Circ, self.CircQueued, self.Pack,
		self.TimeUsed.Nanoseconds(),
	)
}

func (self *Brief) Readable() string {
	buf := new(bytes.Buffer)
	fmt.Fprintf(buf, "day: %d\n", self.Nday)
	fmt.Fprintf(buf, "circ: %s\n", self.perc(self.Circ))
	fmt.Fprintf(buf, "circ-pend: %s\n", self.perc(self.CircQueued))
	fmt.Fprintf(buf, "pack: %s\n", self.perc(self.Pack))
	fmt.Fprintf(buf, "(in %v)\n", self.TimeUsed)

	return buf.String()
}
