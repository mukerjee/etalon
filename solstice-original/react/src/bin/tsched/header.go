package main

import (
	"encoding/json"

	. "react/sim/config"
)

type Header struct {
	Pattern string

	Nhost     int
	LinkBw    uint64
	PackBw    uint64
	WeekLen   uint64
	NightLen  uint64
	MinDayLen uint64
	AvgDayLen uint64

	Norm uint64

	SeedStart int64
	Runs      int
}

func NewHeader() *Header {
	ret := new(Header)
	ret.Pattern = "sym"

	ret.Nhost = Nhost
	ret.LinkBw = LinkBw
	ret.PackBw = PackBw
	ret.WeekLen = WeekLen
	ret.NightLen = NightLen
	ret.MinDayLen = MinDayLen
	ret.AvgDayLen = AvgDayLen

	ret.Norm = 1000

	ret.Runs = 1

	return ret
}

func ParseHeader(dec *json.Decoder) (*Header, error) {
	ret := NewHeader()
	e := dec.Decode(ret)
	return ret, e
}

func (self *Header) Config() {
	SetNhost(self.Nhost)
	LinkBw = self.LinkBw
	PackBw = self.PackBw
	WeekLen = self.WeekLen
	NightLen = self.NightLen
	MinDayLen = self.MinDayLen
	AvgDayLen = self.AvgDayLen
}
