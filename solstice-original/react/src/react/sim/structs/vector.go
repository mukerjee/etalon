package structs

import (
	"bytes"
	"fmt"

	. "react/sim/config"
)

type Vector []uint64

func NewVector() Vector {
	return make([]uint64, Nhost)
}

func NewVectorOf(i uint64) Vector {
	ret := NewVector()
	ret.Assign(i)
	return ret
}

func (v Vector) Clear() {
	for i := 0; i < Nhost; i++ {
		v[i] = 0
	}
}

func (v Vector) Assign(a uint64) {
	for i := 0; i < Nhost; i++ {
		v[i] = a
	}
}

func (v Vector) Max() uint64 {
	ret := uint64(0)

	for i := 0; i < Nhost; i++ {
		if v[i] > ret {
			ret = v[i]
		}
	}

	return ret
}

func (v Vector) SubBy(a uint64) {
	for i := 0; i < Nhost; i++ {
		v[i] = a - v[i]
	}
}

func (v Vector) Cap(a uint64) {
	for i := 0; i < Nhost; i++ {
		if v[i] > a {
			v[i] = a
		}
	}
}

func (v Vector) Reach(a uint64) {
	v.Cap(a)
	v.SubBy(a)
}

func (v Vector) String() string {
	buf := new(bytes.Buffer)
	for i := 0; i < Nhost; i++ {
		if i > 0 {
			fmt.Fprint(buf, " ")
		}
		fmt.Fprintf(buf, "%d", v[i])
	}
	return buf.String()
}

func (v Vector) Div(a uint64) {
	for i := 0; i < Nhost; i++ {
		v[i] /= a
	}
}

func (v Vector) Sum() uint64 {
	ret := uint64(0)
	for i := 0; i < Nhost; i++ {
		ret += v[i]
	}
	return ret
}

func (v Vector) Vadd(v2 Vector) {
	for i := 0; i < Nhost; i++ {
		v[i] += v2[i]
	}
}

func (v Vector) Vtrim(v2 Vector) {
	for i := 0; i < Nhost; i++ {
		if v[i] > v2[i] {
			v[i] -= v2[i]
		} else {
			v[i] = 0
		}
	}
}

func (v Vector) Leq(a uint64) bool {
	for i := 0; i < Nhost; i++ {
		if v[i] > a {
			return false
		}
	}
	return true
}
