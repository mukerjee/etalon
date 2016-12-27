package main

import (
	"fmt"

	"react/sim/bvn"
)

type Scheduler struct {
	Align   bool
	Stuffer string
	Slicer  string
}

func (self *Scheduler) stuffer() bvn.Stuffer {
	switch self.Stuffer {
	case "quick":
		return bvn.NewSharpStuffer()
	case "skewed":
		return bvn.NewSkewedStuffer()
	case "flat":
		return bvn.NewFlatStuffer()
	}

	panic(fmt.Errorf("unknown stuffer: %s", self.Stuffer))
}

func (self *Scheduler) slicer() bvn.Slicer {
	switch self.Slicer {
	case "any":
		return bvn.NewAnySlicer()
	case "max-sum":
		return bvn.NewMaxSumSlicer()
	case "max":
		return bvn.NewMaxSlicer()
	case "islip":
		return bvn.NewIslipSlicer()
	case "laura":
		return bvn.NewLauraSlicer()
	}

	panic(fmt.Errorf("unknown slicer: %s", self.Slicer))
}

func (self *Scheduler) makeName() string {
	ret := make([]byte, 3)
	if self.Align {
		ret[2] = 'a'
	} else {
		ret[2] = '-'
	}

	switch self.Slicer {
	case "any":
		ret[1] = 'a'
	case "max-sum":
		ret[1] = 's'
	case "max":
		ret[1] = 'm'
	case "islip":
		ret[1] = 'i'
	case "laura":
		ret[1] = 'l'
	default:
		ret[1] = '~'
	}

	switch self.Stuffer {
	case "quick":
		ret[0] = 'q'
	case "skewed":
		ret[0] = 's'
	case "flat":
		ret[0] = 'f'
	default:
		ret[0] = '~'
	}

	return string(ret)
}

func (self *Scheduler) doStuff() bool {
	if self.Slicer == "islip" || self.Slicer == "laura" {
		return false
	}
	return true
}

func (self *Scheduler) doGreedy() bool {
	/*
		if self.Slicer == "islip" {
			return true
		}
	*/
	if self.Slicer == "islip" || self.Slicer == "laura" {
		return false
	}
	return true
}

func (self *Scheduler) stopCond() int {
	if self.Slicer == "islip" || self.Slicer == "laura" {
		return bvn.StopOnTimeUsedUp
	}
	return bvn.StopOnNotPerfect
}

func (self *Scheduler) Make() *bvn.Scheduler {
	ret := bvn.NewScheduler()
	ret.SafeBandw = false
	ret.DoTrim = true
	ret.DoAlign = self.Align
	ret.DoScale = true
	ret.DoMerge = true
	ret.DoDiscard = false
	ret.DoShuffle = false
	ret.DoInterleave = false
	ret.DoGreedy = self.doGreedy()
	ret.DoStuff = self.doStuff()
	ret.StopCond = self.stopCond()
	ret.FullStuff = false
	ret.Slicer = self.slicer()
	ret.Stuffer = self.stuffer()
	ret.Name = self.makeName()
	return ret
}
