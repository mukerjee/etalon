package pois

import "math"
import "math/rand"
import "sort"

type PoisCounter struct {
	mean   int
	period int

	buf    []int
	points []int
	t      int

	rand *rand.Rand
}

func NewPeriodPoisson(r *rand.Rand, mean int, period int) *PoisCounter {
	assert(period > 0)

	ret := new(PoisCounter)
	ret.mean = mean
	ret.period = period
	ret.buf = make([]int, 0, ret.mean*3)
	ret.points = ret.buf
	ret.rand = r
	ret.t = 0

	return ret
}

func NewPoisson(r *rand.Rand, mean int) *PoisCounter {
	return NewPeriodPoisson(r, mean, 1)
}

func poisson(r *rand.Rand, mean int) int {
	el := math.Exp(float64(-mean))
	ret := 0
	p := float64(1)

	for {
		p *= r.Float64()
		if p <= el {
			break
		}
		ret++
	}
	return ret
}

func (self *PoisCounter) randTime() int {
	return self.rand.Intn(self.period)
}

func (self *PoisCounter) Tick() int {
	if self.period == 1 {
		return poisson(self.rand, self.mean)
	}

	if self.t == 0 {
		assert(len(self.points) == 0)
		points := self.buf
		assert(len(points) == 0)
		n := poisson(self.rand, self.mean)
		for i := 0; i < n; i++ {
			points = append(points, self.randTime())
		}

		sort.Ints(points)
		self.points = points
	}

	ret := 0
	for len(self.points) > 0 {
		p := self.points[0]
		if p > self.t {
			break
		}

		assert(p == self.t)
		self.points = self.points[1:]
		ret++
	}

	self.t++
	self.t %= self.period

	return ret
}
