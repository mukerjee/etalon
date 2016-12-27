package replay

import (
	"bytes"
	"fmt"
)

type Matrix []uint64

func NewMatrix(nhost int) Matrix {
	return Matrix(make([]uint64, nhost*nhost))
}

func (m Matrix) Clear() {
	for i := range m {
		m[i] = 0
	}
}

func (m Matrix) Madd(a Matrix) {
	for i := range m {
		m[i] += a[i]
	}
}

func (m Matrix) Sum() uint64 {
	ret := uint64(0)
	for _, a := range m {
		ret += a
	}
	return ret
}

func (m Matrix) Json() string {
	buf := new(bytes.Buffer)
	fmt.Fprint(buf, "[")

	for i, n := range m {
		if i != 0 {
			fmt.Fprint(buf, ",")
		}
		fmt.Fprintf(buf, "%d", n)
	}

	fmt.Fprint(buf, "]")

	return buf.String()
}
