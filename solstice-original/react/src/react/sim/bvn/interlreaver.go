package bvn

import (
	. "react/sim/config"
)

type bucket struct {
	*Slice
	budget    int64
	nticks    uint64
	ndays     uint64
	avgDayLen uint64
	nLargeDay uint64
}

func newBucket(slice *Slice) *bucket {
	ret := new(bucket)
	ret.nticks = slice.Weight
	ret.Slice = slice
	ret.budget = 0
	ret.ndays = ret.nticks / AvgDayLen
	if ret.ndays == 0 {
		ret.avgDayLen = ret.nticks
		ret.ndays = 1
		ret.nLargeDay = 0
	} else {
		ret.avgDayLen = ret.nticks / ret.ndays
		ret.nLargeDay = ret.nticks % ret.ndays
	}

	return ret
}

type Interleaver struct {
	buckets []*bucket

	nticks uint64
	ndays  uint64
}

func NewInterleaver() *Interleaver {
	ret := new(Interleaver)
	ret.buckets = make([]*bucket, 0, Nlane*2)

	return ret
}

func (self *Interleaver) clear() {
	self.buckets = self.buckets[0:0]
	self.nticks = 0
	self.ndays = 0
}

func (self *Interleaver) Interleave(days []*Slice) []*Slice {
	if len(days) == 1 {
		return days
	}

	self.clear()

	for _, day := range days {
		bucket := newBucket(day)
		self.buckets = append(self.buckets, bucket)
		self.nticks += day.Weight
		self.ndays += bucket.ndays
	}

	ret := make([]*Slice, 0, self.ndays)

	for i := uint64(0); i < self.ndays; i++ {
		ret = append(ret, self.pick())
	}

	return ret
}

func (self *Interleaver) pick() *Slice {
	var max *bucket

	for _, b := range self.buckets {
		if max == nil || b.budget > max.budget {
			max = b
		} else if b.budget == max.budget {
			if b.nticks > b.nticks {
				max = b
			}
		}
	}

	progress := max.avgDayLen
	if max.nLargeDay > 0 {
		max.nLargeDay--
		progress++
	}
	ret := max.Slice.split(progress)

	max.budget -= int64(progress * self.nticks)
	totalBudget := int64(0)
	for _, b := range self.buckets {
		b.budget += int64(progress * b.nticks)
		totalBudget += b.budget
	}
	assert(totalBudget == 0)

	return ret
}
