package structs

import (
	"bytes"
	"fmt"

	"math/rand"
	. "react/sim/config"
)

const InvalidPort = -1

type PortMap []int

func NewPortMap() PortMap {
	ret := PortMap(make([]int, Nhost))
	ret.Clear()
	return ret
}

func DirectPortMap() PortMap {
	ret := PortMap(make([]int, Nhost))
	for i := range ret {
		ret[i] = i
	}

	return ret
}

func RandPortMap(r *rand.Rand) PortMap {
	return PortMap(r.Perm(Nhost))
}

func (self PortMap) Rand(r *rand.Rand) {
	copy(self, RandPortMap(r))
}

func (self PortMap) Clone() PortMap {
	ret := PortMap(make([]int, Nhost))
	for i, m := range self {
		ret[i] = m
	}
	return ret
}

func (self PortMap) Clear() {
	for i := 0; i < Nhost; i++ {
		self[i] = InvalidPort
	}
}

func (self PortMap) FillReverse(ret PortMap) {
	ret.Clear()
	for i := 0; i < Nhost; i++ {
		dest := self[i]
		if dest < 0 {
			continue
		}
		ret[dest] = i
	}
}

func (self PortMap) Equal(other PortMap) bool {
	for i := 0; i < Nhost; i++ {
		if self[i] != other[i] {
			return false
		}
	}

	return true
}

func (self PortMap) Mask(mask Matrix) {
	for row, col := range self {
		if col < 0 {
			continue
		}
		lane := Lane(row, col)
		if mask[lane] == 0 {
			self[row] = InvalidPort
		}
	}
}

func (self PortMap) CopyInto(other PortMap) {
	for i := 0; i < Nhost; i++ {
		other[i] = self[i]
	}
}

func (self PortMap) String() string {
	ret := new(bytes.Buffer)
	first := true
	for row, col := range self {
		if col < 0 {
			continue
		}

		if !first {
			fmt.Fprintf(ret, " ")
		}
		fmt.Fprintf(ret, "%d-%d", row, col)
		first = false
	}

	return ret.String()
}

func (self PortMap) Matrix() Matrix {
	ret := NewMatrix()
	ret.MapInc(self)
	return ret
}

func (self PortMap) Direct() {
	for row := range self {
		self[row] = row
	}
}

func (self PortMap) FillUp() {
	srcs := make([]int, 0, Nhost)
	dests := make([]int, 0, Nhost)
	buf := NewPortMap()
	self.FillReverse(buf)
	for src, dest := range self {
		if dest < 0 {
			srcs = append(srcs, src)
		}
	}
	for dest, src := range buf {
		if src < 0 {
			dests = append(dests, dest)
		}
	}
	assert(len(srcs) == len(dests))

	for i, src := range srcs {
		self[src] = dests[i]
	}
}
