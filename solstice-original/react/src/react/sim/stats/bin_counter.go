package stats

import "fmt"

type BinCounter struct {
	Bins        []int
	WeightedSum int
	Sum         int
	Min         int
	Max         int
}

func NewBinCounter(max int) *BinCounter {
	ret := new(BinCounter)
	ret.Bins = make([]int, max)

	return ret
}

func (self *BinCounter) String() string {
	return fmt.Sprintf("[n=%d sum=%d min=%d max=%d avg=%.2f "+
		"mid=%d 90%%=%d 95%%=%d 99%%=%d]",
		self.Sum, self.WeightedSum, self.Min, self.Max, self.Average(),
		self.Mid(), self.Ninety(), self.NinetyFive(), self.NinetyNine())
}

func (self *BinCounter) Countw(i, w int) {
	assert(i >= 0)
	if w == 0 {
		return
	}
	assert(w > 0)

	if self.Sum == 0 {
		self.Min = i
		self.Max = i
	} else {
		if i < self.Min {
			self.Min = i
		}
		if i > self.Max {
			self.Max = i
		}
	}

	self.WeightedSum += i * w
	self.Sum += w
	max := len(self.Bins)
	if i >= max {
		self.Bins[max-1] += w
	} else {
		self.Bins[i] += w
	}
}

func (self *BinCounter) Count(i int) {
	self.Countw(i, 1)
}

func (self *BinCounter) Average() float64 {
	if self.Sum == 0 {
		return 0
	}

	return float64(self.WeightedSum) / float64(self.Sum)
}

func (self *BinCounter) Percentile(i float64) int {
	target := int(float64(self.Sum) * i)
	if target == 0 {
		return self.Min
	}
	count := 0
	for i, b := range self.Bins {
		count += b
		if count >= target {
			return i
		}
	}

	panic("unreachable")
}

func (self *BinCounter) Percentiles(ps []float64) []int {
	var ret []int
	for p := range ps {
		ret = append(ret, p)
	}

	return ret
}

func (self *BinCounter) Mid() int {
	return self.Percentile(.5)
}

func (self *BinCounter) Ninety() int {
	return self.Percentile(.9)
}

func (self *BinCounter) NinetyFive() int {
	return self.Percentile(.95)
}

func (self *BinCounter) NinetyNine() int {
	return self.Percentile(.99)
}
