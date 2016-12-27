package demand

import (
	"fmt"
	"io"
)

type Vector []uint64

func (self Vector) PrintTo(w io.Writer) error {
	var e error
	for i, b := range self {
		if i != 0 {
			_, e = fmt.Fprintf(w, " ")
			if e != nil {
				return e
			}
		}

		if b == 0 {
			_, e = fmt.Fprintf(w, "-")
		} else {
			_, e = fmt.Fprintf(w, "%d", b)
		}
		if e != nil {
			return e
		}
	}

	_, e = fmt.Fprintln(w)
	if e != nil {
		return e
	}

	return nil
}

func NewVector(nhost int) Vector {
	return Vector(make([]uint64, nhost))
}
