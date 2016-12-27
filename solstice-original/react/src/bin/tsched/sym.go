package main

import (
	. "react/sim/config"
	. "react/sim/structs"

	"encoding/json"
	"math/rand"
)

type Sym struct {
	Line  []uint64
	Nline int
	Noise uint64
}

var _ DemandMaker = new(Sym)

func ParseSym(dec *json.Decoder) (*Sym, error) {
	ret := NewSym()
	e := dec.Decode(ret)
	return ret, e
}

func NewSym() *Sym {
	ret := new(Sym)

	ret.Line = []uint64{LinkBw - PackBw}
	ret.Nline = Nhost
	ret.Noise = 0

	return ret
}

func relativeLane(src, dest int) int {
	return src*Nhost + (src+dest)%Nhost
}

func (self *Sym) _make() Matrix {
	ret := NewMatrix()

	for i := 0; i < self.Nline && i < Nhost; i++ {
		for j, n := range self.Line {
			ret[relativeLane(i, j+1)] = n
		}
	}

	return ret
}

func (self *Sym) Make(norm uint64, r *rand.Rand) Matrix {
	ret := self._make()
	addNoise(ret, r, self.Noise)
	normDown(ret, norm)

	return ret
}
