package main

import (
	"encoding/json"
	"fmt"
	"io/ioutil"
	"math"
	"os"
	"strings"

	"react/sim"
	"react/sim/bvn"
	. "react/sim/config"
	"react/sim/csol"
)

type Setup struct {
	Nhost       int
	LinkBw      uint64
	PackBw      uint64
	WeekLen     uint64
	NightLen    uint64
	MinDayLen   uint64
	AvgDayLen   uint64
	TickPerGrid uint64

	Nbig      int
	Nsmall    int
	FracSmall int
	FracNorm  int
	FillBw    uint64
	Tries     int
	ClearDiag bool
	Noise     int
	DoTrim    bool

	Scan     string
	ScanFrom int
	ScanTo   int
	ScanStep int
	ScanId   int

	Scheduler string
	Slicer    string
	Demand    string

	DutyFactor int

	HybridOracle bool
}

func defaultSetup() *Setup {
	self := new(Setup)

	self.Nhost = 64
	self.LinkBw = 1000
	self.PackBw = 100
	self.WeekLen = 3000
	self.NightLen = 30
	self.MinDayLen = 100
	self.AvgDayLen = 300
	self.TickPerGrid = 1

	self.Nbig = 3
	self.Nsmall = 0
	self.FracSmall = 0
	self.FracNorm = 1000
	self.FillBw = 950
	self.Tries = 1000
	self.ClearDiag = true
	self.Noise = 5
	self.DoTrim = true

	self.Scan = ""
	self.ScanFrom = 0
	self.ScanTo = 0
	self.ScanStep = 1

	self.Scheduler = "lonnie"
	self.Demand = "unify"
	self.Slicer = "any"

	self.DutyFactor = 5

	return self
}

func (self *Setup) Apply() {
	SetNhost(self.Nhost)
	LinkBw = self.LinkBw
	PackBw = self.PackBw
	WeekLen = self.WeekLen
	NightLen = self.NightLen
	MinDayLen = self.MinDayLen
	AvgDayLen = self.AvgDayLen
	TickPerGrid = self.TickPerGrid
}

func (self *Setup) TryLoad() error {
	bytes, e := ioutil.ReadFile("scan.conf")
	if os.IsNotExist(e) {
		return nil
	}

	if e != nil {
		return e
	}

	return json.Unmarshal(bytes, self)
}

func TryLoad() (*Setup, error) {
	self := defaultSetup()
	e := self.TryLoad()
	return self, e
}

func (self *Setup) Next() bool {
	if self.ScanFrom <= self.ScanTo {
		if self.Scan == "" {
			// do nothing
		} else if self.Scan == "frac" {
			self.FracSmall = self.ScanFrom
			if self.Scheduler == "align" {
				self.MinDayLen = self.alignGrid()
				self.AvgDayLen = self.alignGrid() * 3
			}
		} else if self.Scan == "nbig" {
			self.Nbig = self.ScanFrom
		} else if self.Scan == "minday" {
			self.MinDayLen = uint64(self.ScanFrom)
			self.AvgDayLen = 1
		} else if self.Scan == "night" {
			if self.ScanFrom > 0 {
				self.NightLen = uint64(self.ScanFrom)
				if strings.HasPrefix(self.Scheduler, "islip") {
					self.MinDayLen = self.NightLen * uint64(self.DutyFactor)
					self.AvgDayLen = self.NightLen * uint64(self.DutyFactor)
				} else if self.Scheduler == "align" {
					self.MinDayLen = self.alignGrid()
					self.AvgDayLen = self.alignGrid() * 3
				} else if self.Scheduler == "lonnie" {
					self.MinDayLen = self.NightLen * 2
					self.AvgDayLen = self.NightLen * 2
				} else if self.Scheduler == "csol" {
					self.MinDayLen = self.NightLen * 2
					self.AvgDayLen = self.NightLen * 2
				} else {
					self.MinDayLen = self.NightLen + 1
					self.AvgDayLen = self.NightLen * 10
				}
				self.Nbig = int(self.WeekLen / (20 * self.NightLen))
				if self.Nbig > self.Nhost {
					self.Nbig = self.Nhost
				}
			} else {
				self.NightLen = 0
				if strings.HasPrefix(self.Scheduler, "islip") {
					self.MinDayLen = 1
					self.AvgDayLen = 1
				} else {
					self.MinDayLen = 1
					self.AvgDayLen = 10
				}
				self.Nbig = self.Nhost
			}

			if self.Demand == "rand" {
				self.Nbig *= self.Nhost / 8
			} else {
				n := self.Nbig
				self.Nbig = int(n * 1 / 5)
				self.Nsmall = n - self.Nbig
				self.FracSmall = 280
			}
		} else if self.Scan == "nhost" {
			self.Nhost = self.ScanFrom
			if self.Demand == "rand" {
				self.Nbig = self.Nhost * 2
				self.Nsmall = self.Nhost * 10
				println("nbig =", self.Nbig)
			} else {
				panic("demand must be rand")
			}
		} else {
			panic(fmt.Errorf("unknown scan from: %s", self.Scan))
		}

		self.ScanId = self.ScanFrom

		if self.Scan == "nhost" {
			self.ScanFrom *= 2
		} else {
			self.ScanFrom += self.ScanStep
		}
		return true
	}
	return false
}

func (self *Setup) MakeScheduler() (sim.Scheduler, bvn.Slicer) {
	if self.Scheduler == "csol" {
		return csol.New(), nil
	}

	var slicer bvn.Slicer
	sched := bvn.NewScheduler()
	sched.SafeBandw = false
	sched.DoTrim = self.DoTrim
	sched.DoAlign = false
	sched.DoMerge = true
	sched.DoDiscard = false
	sched.DoShuffle = false
	sched.DoInterleave = false
	sched.DoGreedy = true
	sched.DoStuff = true
	sched.DoScale = true
	sched.StopCond = bvn.StopOnNotPerfect
	sched.FullStuff = false
	sched.Stuffer = bvn.NewSharpStuffer()

	switch self.Scheduler {
	case "lonnie", "oracle", "alexc":
		switch self.Slicer {
		case "any":
			slicer = bvn.NewAnySlicer()
		case "maxsum":
			slicer = bvn.NewMaxSumSlicer()
		case "max":
			slicer = bvn.NewMaxSlicer()
		default:
			panic(fmt.Errorf("unknown slicer: %s", self.Slicer))
		}

		sched.DoGreedy = true
		sched.SingleLevel = false
		sched.StopCond = bvn.StopOnNotPerfect
		if self.Scheduler == "alexc" {
			// sched.DecompStuffer = bvn.NewSharpStuffer()
			sched.DecompStuffer = bvn.NewOverStuffer()
		}
	case "islip", "islip4":
		islipSlicer := bvn.NewIslipSlicer()
		if self.Scheduler == "islip4" {
			islipSlicer.Iteration = 4
		}
		slicer = islipSlicer
		sched.DoMerge = false
		sched.DoStuff = false
		sched.DoGreedy = false
		sched.SingleLevel = true
		sched.StopCond = bvn.StopOnTimeUsedUp
	case "laura":
		slicer = bvn.NewLauraSlicer()
		sched.DoGreedy = false
		sched.SingleLevel = true
		sched.StopCond = bvn.StopOnTimeUsedUp
	case "bvn":
		slicer = bvn.NewAnySlicer()
		sched.SingleLevel = true
		sched.StopCond = bvn.StopOnNotPerfect
	case "align":
		slicer = bvn.NewAnySlicer()
		sched.SingleLevel = true
		sched.StopCond = bvn.StopOnNotPerfect
		sched.DoAlign = true
		sched.DoTrim = false
		sched.Grid = self.alignGrid()
		sched.StarveAlign = true
		println("align grid =", sched.Grid)
	default:
		panic(fmt.Errorf("unknown scheduler: %s", self.Scheduler))
	}

	sched.Slicer = slicer
	return sched, slicer
}

// This is searching for an aligning grid for the proper week length
func (self *Setup) alignGrid() uint64 {
	scanBound := uint64(math.Floor(math.Sqrt(float64(self.WeekLen))))
	minBound := self.NightLen * uint64(self.DutyFactor)
	ret := self.WeekLen

	for i := uint64(1); i <= scanBound; i++ {
		j := self.WeekLen / i
		mod := self.WeekLen % i
		if mod != 0 {
			continue
		}

		if i >= minBound && i < ret {
			ret = i
		}
		if j >= minBound && j < ret {
			ret = j
		}
	}

	return ret
}
