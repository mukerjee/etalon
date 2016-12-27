package main

import (
	"fmt"
)

// Feng's way to compute the value
func map_percent(per int) (i, f int) {
	tmp := per

	if tmp == 0 {
		return 512, 0
	}

	left := 100 % tmp
	i = 100 / tmp
	f = 0

	tmp <<= 14
	left <<= 14

	for j := 0; j < 14; j++ {
		f <<= 1
		tmp >>= 1
		if tmp <= left && tmp > 0 {
			f++
			left -= tmp
		}
	}

	return i, f
}

// An easier and more natual way
func map_percentv(per int) int {
	if per == 0 {
		return 512 << 14
	}
	return (100 << 14) / per
}

const DIV100 = 100 << 14
const DIV = 1 << 14

func assert(cond bool) {
	if !cond {
		panic("assert failed")
	}
}

func main() {
	fmt.Println("static u32 RATE_MAP[101] = {")

	for p := 0; p <= 100; p++ {
		i, f := map_percent(p)
		v := map_percentv(p)
		assert((i<<14)+f == v)
		assert(i < (0x1<<10) && f < (0x1<<14))
		fmt.Printf("    0x%08x, ", v)
		fmt.Printf("// %d%% int=%d frac=%d value=%.3f\n", p, i, f, float64(DIV100)/float64(v))

		if (p+1)%5 == 0 {
			fmt.Println()
		}
	}

	fmt.Println("};")
}
