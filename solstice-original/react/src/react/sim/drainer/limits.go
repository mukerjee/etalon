package drainer

import (
	. "react/sim/config"
	. "react/sim/structs"
)

func RowLimits(limits []*Limit, bound uint64) []*Limit {
	for i := 0; i < Nhost; i++ {
		limits = append(limits, RowLimit(i, bound))
	}
	return limits
}

func ColLimits(limits []*Limit, bound uint64) []*Limit {
	for i := 0; i < Nhost; i++ {
		limits = append(limits, ColLimit(i, bound))
	}
	return limits
}

func BandwLimits(limits []*Limit, bandw uint64) []*Limit {
	limits = RowLimits(limits, bandw)
	limits = ColLimits(limits, bandw)
	return limits
}

func RowVarLimits(limits []*Limit, bounds Vector) []*Limit {
	for i := 0; i < Nhost; i++ {
		limits = append(limits, RowLimit(i, bounds[i]))
	}
	return limits
}

func ColVarLimits(limits []*Limit, bounds Vector) []*Limit {
	for i := 0; i < Nhost; i++ {
		limits = append(limits, ColLimit(i, bounds[i]))
	}
	return limits
}

func RowAvailable(limits []*Limit, used Matrix, bandw uint64) []*Limit {
	v := NewVector()
	used.RowSum(v)
	v.Reach(bandw)
	return RowVarLimits(limits, v)
}

func ColAvailable(limits []*Limit, used Matrix, bandw uint64) []*Limit {
	v := NewVector()
	used.ColSum(v)
	v.Reach(bandw)
	return ColVarLimits(limits, v)
}

func Available(limits []*Limit, used Matrix, bandw uint64) []*Limit {
	limits = RowAvailable(limits, used, bandw)
	limits = ColAvailable(limits, used, bandw)
	return limits
}
