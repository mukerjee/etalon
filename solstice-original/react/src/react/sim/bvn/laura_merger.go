package bvn

import (
	. "react/sim/config"
	. "react/sim/structs"
)

type LauraMerger struct {
	rev      PortMap
	searched []bool
}

func NewLauraMerger() *LauraMerger {
	ret := new(LauraMerger)
	ret.rev = NewPortMap()
	ret.searched = make([]bool, Nhost)
	return ret
}

// Based on matrix m, merge m1 and m2, return the merged portmap
func (self *LauraMerger) Merge(m Sparse, m1, m2, ret PortMap) {
	/*
		println("// luara-merge")
		println(m1.String())
		println(m2.String())
	*/
	copy(ret, m1)

	rev := self.rev
	m2.FillReverse(rev)

	searched := self.searched
	for i := range searched {
		searched[i] = false
	}

	for i := range searched {
		if !searched[i] {
			src := i
			sum1 := uint64(0)
			sum2 := uint64(0)

			for {
				assert(!searched[src])
				searched[src] = true

				dest := m1[src]
				sum1 += m.At(src, dest)
				nextSrc := rev[dest]
				sum2 += m.At(nextSrc, dest)

				if nextSrc == i {
					break
				}
				src = nextSrc
			}

			if sum1 < sum2 {
				// need to flip here
				src := i
				for {
					dest := m1[src]
					nextSrc := rev[dest]
					ret[nextSrc] = dest
					if nextSrc == i {
						break
					}
					src = nextSrc
				}
			}
		}
	}
}
