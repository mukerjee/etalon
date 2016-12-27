package bvn

import (
	. "react/sim/config"
	. "react/sim/structs"

	// "fmt"
)

type DecompOpts struct {
	Slicer        Slicer
	DecompStuffer SparseStuffer
	SingleLevel   bool
	StopCond      int

	// these are set by the decomposer
	DecompMin  uint64 // for trimming small ones
	DecompMax  uint64 // for bounding slot length
	DecompWeek uint64
}

const (
	StopOnNotPerfect = iota
	StopOnZero
	StopOnTimeUsedUp
)

type Decomposer struct {
	d     Matrix
	sp    Sparse
	input Sparse
	pm    PortMap
}

func NewDecomposer() *Decomposer {
	ret := new(Decomposer)
	ret.sp = NewSparse()
	ret.input = NewSparse()
	ret.pm = NewPortMap()
	ret.d = NewMatrix()

	return ret
}

func (self *Decomposer) findSlice(opts *DecompOpts,
	in, d Sparse, id int, min uint64) (*Slice, int) {
	slicer := opts.Slicer
	ret := NewSlice(id)
	pm := self.pm
	pm.Clear()

	nmatch := slicer.Slice(in, pm)
	weight := uint64(0)
	if nmatch > 0 {
		weight = in.MinInMap(pm)

		if opts.DecompStuffer != nil {
			// TODO: might have better way to do this
			minWeight := d.MinInMap(pm)
			maxWeight := weight
			if minWeight < min {
				minWeight = min
			}
			if maxWeight > minWeight {
				delta := maxWeight - minWeight
				weight = minWeight + delta/uint64(Nhost)
			}
		}

		assert(weight > 0)
		copy(ret.PortMap, pm)
	}

	ret.Weight = weight

	return ret, nmatch
}

func (self *Decomposer) thres(d Matrix, min uint64) Sparse {
	ret := self.sp
	ret.Clear()

	for i, n := range d {
		if n >= min {
			ret.Set(i, n)
		}
	}

	return ret
}

func (self *Decomposer) thresSparse(d Sparse, min uint64) Sparse {
	return self.sp.ThresFrom(d, min)
}

func (self *Decomposer) thresInput(d Sparse, min uint64) Sparse {
	return self.input.ThresFrom(d, min)
}

func (self *Decomposer) sub(sp Sparse, slice *Slice, min uint64) {
	w := slice.Weight
	for row, col := range slice.PortMap {
		if col < 0 {
			continue
		}
		sp.SubTrim(row, col, w, min)
	}
}

func (self *Decomposer) decompose(d Matrix, min uint64,
	opts *DecompOpts) []*Slice {
	// d must be a doubly stochastic matrix here

	var sp Sparse

	if opts.DecompStuffer != nil {
		// we will stuff on every slicing
		sp = self.thres(d, 1)
	} else {
		sp = self.thres(d, min)
	}

	// fmt.Println(d.MatrixStr())
	// fmt.Println(sp.Matrix().MatrixStr())

	return self._decompose(sp, min, opts)
}

func (self *Decomposer) _decompose(sp Sparse, min uint64,
	opts *DecompOpts) []*Slice {
	ret := make([]*Slice, 0, Nlane*2)
	id := 0
	total := uint64(0)

	for {
		// logln("residue: ", sp.Matrix())
		input := sp
		if opts.DecompStuffer != nil {
			input = self.input.CopyFrom(sp)
			opts.DecompStuffer.StuffSparse(input, 0)
			input.Thres(min)
		}

		slice, nmatch := self.findSlice(opts, input, sp, id, min)

		if slice.Weight < min {
			slice.Weight = min
		}
		assert(slice.Weight > 0)
		if opts.DecompMax > 0 && slice.Weight > opts.DecompMax {
			slice.Weight = opts.DecompMax
		}

		stop := false
		switch opts.StopCond {
		case StopOnNotPerfect:
			stop = nmatch < Nhost
		case StopOnZero:
			stop = (nmatch == 0)
		case StopOnTimeUsedUp:
			stop = (total+slice.Weight > opts.DecompWeek)
		default:
			panic("bug")
		}

		if stop {
			break
		}

		// logln("slice: ", slice)
		if opts.DecompStuffer == nil {
			self.sub(sp, slice, min)
		} else {
			self.sub(sp, slice, 0)
		}
		ret = append(ret, slice)
		total += slice.Weight
		id++
	}

	return ret
}

func (self *Decomposer) makeLevels(opts *DecompOpts) []uint64 {
	levels := make([]uint64, 0, 10)
	levels = append(levels, opts.DecompMin)
	assert(opts.DecompMin > 0)
	base := opts.DecompMin

	for {
		base *= 2
		if base < WeekLen/2 {
			levels = append(levels, base)
		} else {
			break
		}
	}

	return levels
}

func (self *Decomposer) levelDecompose(d Matrix, opts *DecompOpts) []*Slice {
	copy(self.d, d)

	levels := self.makeLevels(opts)
	// fmt.Println(levels)

	ret := make([]*Slice, 0, 1024)
	nlevel := len(levels)
	for i := range levels {
		level := levels[nlevel-1-i]
		// logln("demand: ", self.d)
		slices := self.decompose(self.d, level, opts)
		// fmt.Println("level: ", level)
		for _, s := range slices {
			// fmt.Println("slice: ", s.PortMap, s.Weight)
			self.d.SubInMap(s.PortMap, s.Weight)
		}
		ret = append(ret, slices...)
	}

	return ret
}

func (self *Decomposer) Decompose(d Matrix, opts *DecompOpts) []*Slice {
	if opts.SingleLevel || opts.StopCond == StopOnTimeUsedUp {
		// a slot based decomposer, then just use DecompMin
		copy(self.d, d)
		return self.decompose(self.d, opts.DecompMin, opts)
	}

	return self.levelDecompose(d, opts)
}

func (self *Decomposer) DecomposeSparse(d Sparse,
	opts *DecompOpts) []*Slice {
	if opts.SingleLevel || opts.StopCond == StopOnTimeUsedUp {
		panic("not supported")
	}

	levels := self.makeLevels(opts)
	ret := make([]*Slice, 0, 1024)
	nlevel := len(levels)
	for i := range levels {
		level := levels[nlevel-1-i]
		slices := self.decomposeSparse(d, level, opts)
		for _, s := range slices {
			d.SubInMap(s.PortMap, s.Weight)
		}
		ret = append(ret, slices...)
	}

	return ret
}

func (self *Decomposer) decomposeSparse(d Sparse, min uint64,
	opts *DecompOpts) []*Slice {
	sp := self.thresSparse(d, min)
	return self._decompose(sp, min, opts)
}
