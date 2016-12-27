package demand

import (
	"fmt"
	"io"
)

type Period struct {
	N uint64
	D []Vector
}

func NewPeriod(nhost int, n uint64) *Period {
	ret := new(Period)
	ret.N = n
	ret.D = newMatrix(nhost)
	return ret
}

func newMatrix(nhost int) []Vector {
	ret := make([]Vector, nhost)
	for i := range ret {
		ret[i] = NewVector(nhost)
	}
	return ret
}

func (self *Period) PrintTo(w io.Writer) error {
	_, e := fmt.Fprintf(w, "t=%d\n", self.N)
	if e != nil {
		return e
	}

	for _, v := range self.D {
		e = v.PrintTo(w)
		if e != nil {
			return e
		}
	}

	return nil
}
