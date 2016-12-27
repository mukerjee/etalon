package drainer

import (
	. "react/sim/config"
	. "react/sim/structs"
	"testing"
)

func TestDrainer(t *testing.T) {
	SetNhost(2)

	_1 := func(data Matrix, lanes []int, bound uint64,
		drained Matrix) {
		lim := NewLimit(lanes, bound)
		drainer := NewDrainer([]*Limit{lim})
		result := drainer.Pull(data)
		if !result.Equals(drained) {
			t.Fatal("drain", data, lim, result, drained)
		}
	}

	data := Matrix{10, 10, 0, 0}
	lanes := []int{0, 1}

	_1(data, lanes, 10, Matrix{5, 5, 0, 0})
	_1(data, lanes, 9, Matrix{4, 4, 0, 0})
	_1(data, lanes, 1, Matrix{0, 0, 0, 0})
	_1(data, lanes, 0, Matrix{0, 0, 0, 0})
	_1(data, lanes, 30, Matrix{10, 10, 0, 0})

	lanes = []int{0, 1, 2}
	_1(data, lanes, 10, Matrix{5, 5, 0, 0})
	_1(data, lanes, 9, Matrix{4, 4, 0, 0})
	_1(data, lanes, 1, Matrix{0, 0, 0, 0})
	_1(data, lanes, 30, Matrix{10, 10, 0, 0})

	lanes = []int{} // no actual bounding
	_1(data, lanes, 10, Matrix{10, 10, 0, 0})
	_1(data, lanes, 30, Matrix{10, 10, 0, 0})

	lanes = []int{1}
	_1(data, lanes, 5, Matrix{10, 5, 0, 0})
	_1(data, lanes, 30, Matrix{10, 10, 0, 0})

	lanes = []int{2, 3} // bounding empty
	_1(data, lanes, 10, Matrix{10, 10, 0, 0})

	_2 := func(data Matrix, lim1 *Limit, lim2 *Limit,
		drained Matrix) {
		drainer := NewDrainer([]*Limit{lim1, lim2})
		result := drainer.Drain(data)

		if !result.Equals(drained) {
			t.Fatal("drain", data, lim1, lim2,
				result, drained)
		}
	}

	lim1 := NewLimit([]int{0, 1}, 10)
	lim2 := NewLimit([]int{0, 2}, 10)
	_2(Matrix{5, 5, 5, 5}, lim1, lim2, Matrix{5, 5, 5, 5})
	_2(Matrix{10, 10, 10, 10}, lim1, lim2, Matrix{5, 5, 5, 10})
	_2(Matrix{10, 2, 3, 1}, lim1, lim2, Matrix{7, 2, 3, 1})
}

func TestLinkDrinaer(t *testing.T) {
	SetNhost(2)

	_1 := func(data Matrix, bound uint64, drained Matrix) {
		drainer := NewUplinkDrainer(bound)
		result := drainer.Pull(data)
		if !result.Equals(drained) {
			t.Fatal("drain", data, bound, result, drained)
		}
	}

	data := Matrix{10, 9, 0, 0}
	_1(data, 10, Matrix{5, 5, 0, 0})
	_1(data, 5, Matrix{2, 2, 0, 0})
	_1(data, 19, Matrix{10, 9, 0, 0})
	_1(data, 18, Matrix{9, 9, 0, 0})
	_1(data, 20, Matrix{10, 9, 0, 0})
}
