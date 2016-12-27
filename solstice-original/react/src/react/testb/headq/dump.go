package headq

import (
	"fmt"
	"react/testb/setup"
	"strconv"
	"strings"
)

type Dump struct {
	Time     uint64
	From, To uint16
	SeqNo    uint32
	Chan     int
}

func (self *Dump) Lane() int {
	return int(self.From*setup.Nhost + self.To)
}

func (self *Dump) String() string {
	return fmt.Sprintf("%d %d %d %d // chan=%s, ",
		self.Time,
		self.From,
		self.To,
		self.SeqNo,
	)
}

func ParseDump(line string, c int) (*Dump, error) {
	ret := new(Dump)
	e := ret.parse(line)
	if e != nil {
		return nil, e
	}
	ret.Chan = c
	return ret, nil
}

func (self *Dump) parse(line string) error {
	fields := strings.Fields(line)
	if len(fields) != 4 {
		return fmt.Errorf("invalid dump line: %s", line)
	}

	i, e := strconv.ParseUint(fields[0], 10, 64)
	if e != nil {
		return e
	}
	self.Time = uint64(i)

	i, e = strconv.ParseUint(fields[1], 10, 16)
	if e != nil {
		return e
	}
	self.From = uint16(i)

	i, e = strconv.ParseUint(fields[2], 10, 16)
	if e != nil {
		return e
	}
	self.To = uint16(i)

	i, e = strconv.ParseUint(fields[3], 10, 32)
	if e != nil {
		return e
	}
	self.SeqNo = uint32(i)

	return nil
}

func (self *Dump) Tick() uint64 {
	return self.Time / setup.NanosPerTick
}
