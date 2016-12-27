package alexf

import (
	. "react/sim/config"
	. "react/sim/structs"

	"math"
	"math/rand"
)

func assert(cond bool) {
	if !cond {
		panic("bug")
	}
}

// Basic max scheduler
func Slice1(m Matrix) Matrix {
	a := NewMatrix()
	rows := make([]bool, Nhost)
	cols := make([]bool, Nhost)

	for n := 0; n < Nhost; n++ {
		val := -1
		valRow := -1
		valCol := -1

		for i := 0; i < Nhost; i++ {
			if rows[i] {
				continue
			}

			for j := 0; j < Nhost; j++ {
				if cols[j] {
					continue
				}

				lane := Lane(i, j)
				if int(m[lane]) > val {
					val = int(m[lane])
					valRow = i
					valCol = j
				}
			}
		}

		assert(valRow >= 0)
		assert(valCol >= 0)

		a[Lane(valRow, valCol)] = 1
		rows[valRow] = true
		cols[valCol] = true
	}

	return a
}

// Basic max row greedy shuffle scheduler
func Slice2(m Matrix) Matrix {
	a := NewMatrix()
	perm := rand.Perm(int(Nhost))
	cols := make([]bool, Nhost)

	for _, i := range perm {
		val := -1
		valRow := -1
		valCol := -1

		for j := 0; j < Nhost; j++ {
			if cols[j] {
				continue
			}

			lane := Lane(i, j)
			if int(m[lane]) > val {
				val = int(m[lane])
				valRow = i
				valCol = j
			}
		}

		assert(valRow >= 0)
		assert(valCol >= 0)

		a[Lane(valRow, valCol)] = 1
		cols[valCol] = true
	}

	return a
}

// Orders-of-magnitude max tie-break row greedy shuffle scheduler
func Slice3(m Matrix) Matrix {
	order := func(e uint64) int {
		return int(math.Floor(math.Log2(float64(e) + 1)))
	}

	a := NewMatrix()
	cols := make([]bool, Nhost)

	perm := rand.Perm(Nhost)

	for i := range perm {
		highestBit := 0

		for j := 0; j < Nhost; j++ {
			if cols[j] {
				continue
			}

			b := order(m[Lane(i, j)])
			if b > highestBit {
				highestBit = b
			}
		}

		var largest []int

		for j := 0; j < Nhost; j++ {
			if cols[j] {
				continue
			}

			b := order(m[Lane(i, j)])
			if b == highestBit {
				largest = append(largest, j)
			}
		}

		assert(len(largest) > 0)
		pick := largest[rand.Intn(len(largest))]

		a[Lane(i, pick)] = 1
		cols[pick] = true
	}

	return a
}
