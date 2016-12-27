package main

import (
	"encoding/json"
	"fmt"
	"io"
	"log"
	"math/rand"
	"os"
	"strings"
	"time"

	"react/sim/bvn"
	. "react/sim/config"
	. "react/sim/structs"
)

type Run struct {
	Header      *Header
	DemandMaker DemandMaker
	Schedulers  *Schedulers

	Demand       Matrix
	NormedDemand Matrix
	BriefHeader  *BriefHeader
}

func LoadRun(path string) (*Run, error) {
	fin, e := os.Open(path)
	if e != nil {
		return nil, e
	}

	ret, e := DecodeRun(fin)
	if e != nil {
		return nil, e
	}

	e = fin.Close()
	if e != nil {
		return nil, e
	}

	return ret, nil
}

func ParseRun(s string) (*Run, error) {
	in := strings.NewReader(s)
	return DecodeRun(in)
}

func DecodeRun(in io.Reader) (*Run, error) {
	var e error
	ret := new(Run)

	dec := json.NewDecoder(in)
	ret.Header, e = ParseHeader(dec)
	if e != nil {
		return ret, e
	}

	ret.Header.Config()

	e = ret.parseMaker(dec)
	if e != nil {
		return ret, e
	}

	ret.Schedulers, e = ParseSchedulers(dec)
	if e != nil {
		return ret, e
	}

	return ret, nil
}

func (self *Run) parseMaker(dec *json.Decoder) error {
	var e error

	pat := self.Header.Pattern

	switch pat {
	case "sym":
		self.DemandMaker, e = ParseSym(dec)
	case "flows":
		self.DemandMaker, e = ParseFlows(dec)
	default:
		return fmt.Errorf("unknown pattern: %s", pat)
	}

	return e
}

func normalized(m Matrix) Matrix {
	ret := m.Clone()
	ret.Div(WeekLen)
	return ret
}

func perc(a, b uint64) float64 {
	if b == 0 {
		return 0
	}
	return float64(a) / float64(b) * 100
}

func (self *Run) MakeDemand(seed int64) {
	if seed == 0 {
		seed = time.Now().UnixNano()
	}

	r := rand.New(rand.NewSource(seed))
	demand := self.DemandMaker.Make(self.Header.Norm, r)
	self.NormedDemand = demand.Clone()
	demand.Mul(LinkBw * WeekLen)
	demand.Div(self.Header.Norm)

	self.Demand = demand
	self.BriefHeader = self.makeHeader(seed)
}

func countBytes(d Matrix, days []*Day, bandw Matrix, ret *Brief) {
	served := NewMatrix()
	t := NewMatrix()

	for _, day := range days {
		if day.Len <= NightLen {
			continue
		}

		copy(t, day.Sched)
		t.Mul(day.Len - NightLen)
		t.Mmul(bandw)
		served.Madd(t)
	}

	for i, b := range served {
		if b == 0 { // packet lane
			ret.Pack += d[i]
		} else if b < d[i] {
			ret.Circ += b
			ret.CircQueued += d[i] - b
		} else if d[i] > 0 {
			ret.Circ += d[i]
		}
	}
}

func (self *Run) RunSched(s *bvn.Scheduler) *Brief {
	ret := new(Brief)
	ret.Header = self.BriefHeader
	ret.SchedName = s.Name

	demand := self.Demand.Clone()

	tstart := time.Now()
	days, bandw := s.Schedule(nil, demand, WeekLen)
	ret.TimeUsed = time.Since(tstart)

	ret.Nday = len(s.DayLens)

	countBytes(demand, days, bandw, ret)

	return ret
}

func (self *Run) makeHeader(seed int64) *BriefHeader {
	if self.Demand == nil {
		panic("demand not ready")
	}

	h := new(BriefHeader)
	h.Demand = self.Demand.Sum()
	h.Capacity = LinkBw * WeekLen * uint64(Nhost)
	h.Seed = seed

	return h
}

func (self *Run) run(scheds []*bvn.Scheduler, lout io.Writer) {
	fmt.Fprintln(lout, self.BriefHeader)
	for _, sched := range scheds {
		b := self.RunSched(sched)
		fmt.Fprintln(lout, b)
	}
}

func (self *Run) Run(lout io.Writer) {
	ticker := time.Tick(time.Second)
	n := self.Header.Runs
	scheds := self.Schedulers.Make()

	names := make([]string, len(scheds))
	for i, s := range scheds {
		names[i] = s.Name
	}
	fmt.Fprintln(lout, strings.Join(names, " "))
	r := rand.New(rand.NewSource(self.Header.SeedStart))

	for i := 0; i < n; i++ {
		seed := int64(0)
		if self.Header.SeedStart != 0 {
			for {
				seed = r.Int63()
				if seed != 0 {
					break
				}
			}
		}
		self.MakeDemand(seed)
		self.run(scheds, lout)
		fmt.Fprintln(lout)

		if len(ticker) > 0 {
			<-ticker
			log.Printf("%d/%d", i, n)
		}
	}
}
