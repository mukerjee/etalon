package main

import (
	. "react/sim/config"
	. "react/sim/structs"

	"math/rand"
)

func addNoise(m Matrix, r *rand.Rand, noise uint64) {
	if noise == 0 {
		return
	}

	for i := 0; i < Nhost; i++ {
		base := i * Nhost
		for j := 0; j < Nhost; j++ {
			if i == j {
				continue
			}
			lane := base + j
			m[lane] += uint64(r.Intn(int(noise)))
		}
	}
}

func addNoiseBudget(m Matrix, r *rand.Rand, budget uint64) {
	for budget > 0 {
		lane := randLane(r)
		m[lane]++
		budget--
	}
}
