package bvn

import (
	. "react/sim/config"
	. "react/sim/structs"

	"math"
	"math/rand"
	"sort"
)

// From paper:
// Efficient Randomized Algorithms for Input-Queued Switch Scheduling
// Devavrat Shah, et.al.
type LauraSlicer struct {
	MaxSumBoot   bool
	maxSumSlicer *MaxSumSlicer
	Rand         *rand.Rand
	merger       *LauraMerger

	history []PortMap // past matchings
	tries   []PortMap // random tries

	// buffer for Random
	destMap PortMap
	srcs    []int
	dests   []int
	edges   []*Edge

	// parameters
	Heta   uint64
	Iter   int // number of random iters, called I in the paper
	Window int // called S in the paper
	Tries  int // called V in the paper
}

var _ Slicer = new(LauraSlicer)

const hetaNorm = 1024

func NewLauraSlicer() *LauraSlicer {
	ret := new(LauraSlicer)
	ret.maxSumSlicer = NewMaxSumSlicer()
	ret.merger = NewLauraMerger()
	ret.MaxSumBoot = true

	ret.Rand = timeRand()
	ret.Heta = hetaNorm / 2

	ret.Iter = 1
	if Nhost >= 2 {
		ret.Iter = int(math.Log2(float64(Nhost)))
	}
	ret.Window = 2 // S
	ret.Tries = 1  // T

	ret.history = make([]PortMap, 0, 10)
	ret.tries = make([]PortMap, 0, 10)
	ret.destMap = NewPortMap()
	ret.srcs = make([]int, 0, Nhost)
	ret.dests = make([]int, 0, Nhost)
	ret.edges = make([]*Edge, Nhost)
	for i := range ret.edges {
		ret.edges[i] = new(Edge)
	}

	return ret
}

type Edge struct {
	src    int
	dest   int
	weight uint64
}

type Edges []*Edge

func (s Edges) Len() int           { return len(s) }
func (s Edges) Swap(i, j int)      { s[i], s[j] = s[j], s[i] }
func (s Edges) Less(i, j int) bool { return s[i].weight > s[j].weight }

// sort in desc order

// the Random procedure in Laura, generates a random pormap
// that should have about O(self.Iter) big elements in it
func (self *LauraSlicer) Random(m Sparse, pm PortMap) {
	srcMap := pm
	destMap := self.destMap // get the buffer
	destMap.Clear()
	nmatched := 0

	for i := 0; i < self.Iter; i++ {
		// println("- iter", i)
		srcs := self.srcs[0:0]   // init unmatched srcs
		dests := self.dests[0:0] // init unmatched dests

		// fill in unmatched srcs
		for src, dest := range srcMap {
			if dest < 0 {
				srcs = append(srcs, src)
			}
		}
		// fill in unmatched dests
		for dest, src := range destMap {
			if src < 0 {
				dests = append(dests, dest)
			} else {
				assert(srcMap[src] == dest)
			}
		}

		// randomly match srcs and dest
		assert(len(srcs) == len(dests))
		n := len(srcs)
		assert(n > 0)
		order := self.Rand.Perm(n)
		edges := self.edges
		sum := uint64(0)
		for i, j := range order {
			e := edges[i]
			e.src = srcs[i]
			e.dest = dests[j]
			e.weight = m.At(e.src, e.dest)
			sum += e.weight
		}
		edges = edges[0:n]

		/*
			for i, e := range edges {
				println("  match", i, "-", e.src, e.dest, e.weight)
			}
		*/

		// if not last, we only pick some of them
		isLast := i == (self.Iter - 1)
		if !isLast {
			sort.Sort(Edges(edges))
			if edges[0].weight == 0 {
				// an empty match, so don't bother
				continue
			}
		}

		// apply the matched edges
		acc := uint64(0)
		bar := sum * self.Heta / 1024
		for _, e := range edges {
			acc += e.weight
			srcMap[e.src] = e.dest
			destMap[e.dest] = e.src
			// println("  ADD match", e.src, e.dest, e.weight)
			nmatched++
			if !isLast && acc >= bar {
				break
			}
		}

		if nmatched == Nhost {
			break
		}
	}
}

func (self *LauraSlicer) Slice(m Sparse, pm PortMap) int {
	if len(self.history) == 0 {
		ret := 0
		if self.MaxSumBoot {
			ret = self.maxSumSlicer.Slice(m, pm)
			this := pm.Clone()
			this.FillUp()

			self.history = append(self.history, this)
		} else {
			this := RandPortMap(self.Rand)
			for src, dest := range this {
				if m.At(src, dest) > 0 {
					pm[src] = dest
					ret++
				}
			}
			self.history = append(self.history, this)
		}

		return ret
	}

	// generate the random tries
	for len(self.tries) < self.Tries {
		self.tries = append(self.tries, NewPortMap())
	}
	tries := self.tries[:self.Tries]
	for _, try := range tries {
		try.Clear()
		self.Random(m, try)
		// println("try:", try.String())
	}

	var merged, maxMerged PortMap
	maxSum := uint64(0)

	for _, his := range self.history {
		for _, try := range tries {
			if merged == nil {
				merged = NewPortMap()
			}
			self.merger.Merge(m, his, try, merged)

			// do some comparing here
			sum := uint64(0)
			for src, dest := range merged {
				sum += m.At(src, dest)
			}
			if maxMerged == nil || sum > maxSum {
				maxMerged, merged = merged, maxMerged
			}
		}
	}

	assert(maxMerged != nil)

	// append history
	self.history = append(self.history, maxMerged)
	if len(self.history) > self.Window {
		// move on
		assert(len(self.history) == self.Window+1)
		for i := 0; i < self.Window; i++ {
			self.history[i] = self.history[i+1]
		}
		self.history = self.history[:self.Window]
	}

	// count number of matched
	ret := 0
	// println("maxMerged:", maxMerged.String())
	for src, dest := range maxMerged {
		// println(src, dest, m.At(src, dest))
		if m.At(src, dest) > 0 {
			pm[src] = dest
			ret++
		}
	}

	return ret
}

func (self *LauraSlicer) Reset() {
	self.history = self.history[0:0] // clear history
	self.Rand = rand.New(rand.NewSource(0))
}
