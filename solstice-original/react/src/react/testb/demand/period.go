package demand

import (
	"react/testb/setup"

	dem "react/demand"
)

func newPeriod(n uint64) *dem.Period {
	return dem.NewPeriod(setup.Nhost, n)
}

func P4r(n uint64, r1, r2, r3 uint64) *dem.Period {
	ret := newPeriod(n)

	for i, v := range ret.D {
		v[(i+1)%4] = r1
		v[(i+2)%4] = r2
		v[(i+3)%4] = r3
	}

	return ret
}

func P8r(n uint64, id int, r1, r2, r3,
	r4, r5, r6, r7 uint64) *dem.Period {
	ret := newPeriod(n)

	for i, v := range ret.D {
		v[(i+1)%8] = r1
		v[(i+2)%8] = r2
		v[(i+3)%8] = r3
		v[(i+4)%8] = r4
		v[(i+5)%8] = r5
		v[(i+6)%8] = r6
		v[(i+7)%8] = r7
	}

	return ret
}

func P0(n uint64) *dem.Period {
	return newPeriod(n)
}
