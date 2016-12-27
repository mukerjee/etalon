package bvn

import (
	"math"

	. "react/sim/config"
	. "react/sim/structs"
)

const (
	farAway     = math.MaxInt64
	distStation = math.MaxInt64 / 4096
)

type MaxSumSlicer struct {
	portMap         PortMap
	portMapReversed PortMap
	laneBuf         []int
	distanceLeft    []int64
	distanceRight   []int64
	lastFrom        PortMap

	AcceptShorter bool
}

var _ Slicer = new(MaxSumSlicer)

func NewMaxSumSlicer() *MaxSumSlicer {
	ret := new(MaxSumSlicer)

	ret.portMapReversed = NewPortMap()
	ret.laneBuf = make([]int, Nlane)
	ret.distanceLeft = make([]int64, Nhost)
	ret.distanceRight = make([]int64, Nhost)
	ret.lastFrom = NewPortMap()

	return ret
}

func clearDistance(dists []int64) {
	for i := 0; i < Nhost; i++ {
		dists[i] = farAway
	}
}

func clearLast(lasts []int) {
	for i := 0; i < Nhost; i++ {
		lasts[i] = InvalidPort
	}
}

// Look for shortest path from a dest to an unmatched source. A matched
// edge is a directed edge from its source to dest with negative weight,
// an unmatched edge is a directed edge from its dest to source with
// positive weight. A shortest path is then a way to extend the maximum
// weighted matching.
//
// We search for the shortest path here using Bellman-Ford algorithm,
// which complexity is O((nmatch + 1) * (|E| - nmatch)). Should be
// pretty fast when |E| is small (i.e. the matrix is sparse)
func (self *MaxSumSlicer) findFor(data Sparse, dest int, nmatch int) bool {
	// logln("findFor:", dest, self.portMap)

	clearDistance(self.distanceLeft)
	clearDistance(self.distanceRight)
	self.lastFrom.Clear()

	self.distanceRight[dest] = 0

	for i := 0; i <= nmatch; i++ { // Nhost rounds of iterating
		nothingUpdated := true
		for col, scol := range data {
			for row, n := range scol {
				// logln("visit: ", row, col, n)
				// logln(self.distanceLeft, "//distLeft")
				// logln(self.distanceRight, "//distRight")

				assert(n > 0)
				s := row
				d := col

				mapDest := self.portMap[s]

				if mapDest == d {
					continue // this is a matched edge
				}

				dright := self.distanceRight[d]
				if dright == farAway {
					continue // not reachable yet
				}

				dleft := dright - int64(n)
				if !self.AcceptShorter {
					dleft -= distStation
				}

				if dleft >= self.distanceLeft[s] {
					continue
				}

				nothingUpdated = false
				self.distanceLeft[s] = dleft
				self.lastFrom[s] = d

				if mapDest == InvalidPort {
					continue
				}

				// s is matched left node
				// so extend back to its matching right node
				e := data.At(s, mapDest)
				assert(e > 0)
				self.distanceRight[mapDest] = dleft + int64(e)
				if !self.AcceptShorter {
					self.distanceRight[mapDest] += distStation
				}
			}
		}

		if nothingUpdated {
			break
		}
	}

	// logln(self.distanceLeft)
	// logln(self.distanceRight)

	// searching for best pairing (shortest path to an unmatched src)
	minSrcDist := int64(0)
	minSrcNode := InvalidPort
	for s, d := range self.portMap {
		if d != InvalidPort {
			continue
		}

		dist := self.distanceLeft[s]
		if dist < minSrcDist {
			minSrcNode = s
			minSrcDist = dist
		}
	}

	// searching for best pairing (shortest path to a matched dest)
	minDestDist := int64(0)
	minDestNode := InvalidPort
	for _, d := range self.portMap {
		if d == InvalidPort {
			continue
		}

		dist := self.distanceRight[d]
		if dist < minDestDist {
			minDestNode = d
			minDestDist = dist
		}
	}

	if minSrcNode == InvalidPort && minDestNode == InvalidPort {
		assert(minSrcDist == 0 && minDestDist == 0)
		return false // cannot find better matching that matches dest
	}

	if minSrcDist < minDestDist {
		assert(minSrcDist < 0)
		assert(minSrcNode != InvalidPort)

		self.portMap.FillReverse(self.portMapReversed)

		// traverse back the shortest path and extend the maximum matching
		node := minSrcNode
		assert(self.portMap[minSrcNode] == InvalidPort)
		for {
			mapNode := self.lastFrom[node]
			assert(mapNode != InvalidPort)
			self.portMap[node] = mapNode

			if mapNode == dest {
				assert(self.portMapReversed[dest] == InvalidPort)
				break
			}

			node = self.portMapReversed[mapNode]
		}

		return true
	} else {
		// augmented path ends at a dest
		// the number of matches does not change, but the
		// path will be flipped
		// logln("mindest:", minDestDist, minDestNode)
		assert(minDestDist < 0)
		assert(minDestNode != InvalidPort)
		self.portMap.FillReverse(self.portMapReversed)
		node := self.portMapReversed[minDestNode]

		for {
			mapNode := self.lastFrom[node]
			assert(mapNode != InvalidPort)

			self.portMap[node] = mapNode
			if mapNode == dest {
				assert(self.portMapReversed[dest] == InvalidPort)
				break
			}

			node = self.portMapReversed[mapNode]
		}

		return false
	}
}

func (self *MaxSumSlicer) Slice(m Sparse, pm PortMap) int {
	// panic("this does not always find a perfect matching")

	// bipartie maximum weighted matching
	self.portMap = pm

	nmatch := 0
	for i := 0; i < Nhost; i++ {
		// logln(pm, "// pm")
		if !self.findFor(m, i, nmatch) {
			continue
		}
		nmatch++
	}

	return nmatch
}

func (self *MaxSumSlicer) Reset() {}
