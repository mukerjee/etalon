package density

import (
	"bytes"
	"fmt"
	"io"
	"sort"

	"react/sim/config"
	"react/sim/structs"
)

var BigThres uint64 = 5000 // slightly more than 3 MTUs

type Density struct {
	NonZeroElements   int
	NonZeroHosts      int
	NonZeroPerHostMax int
	NonZeroPerHost50  int
	NonZeroPerHost90  int

	BigElements   int
	BigHosts      int
	BigPerHostMax int
	BigPerHost50  int
	BigPerHost90  int
}

func (self *Density) PrintTo(out io.Writer) {
	fmt.Fprintf(out, "non-zero elements: %d\n", self.NonZeroElements)
	fmt.Fprintf(out, "non-zero hosts: %d\n", self.NonZeroHosts)
	fmt.Fprintf(out, "non-zero elements per host: %d/%d/%d\n",
		self.NonZeroPerHost50,
		self.NonZeroPerHost90,
		self.NonZeroPerHostMax,
	)
	fmt.Fprintf(out, "big elements: %d\n", self.BigElements)
	fmt.Fprintf(out, "big hosts: %d\n", self.BigHosts)
	fmt.Fprintf(out, "big elements per host: %d/%d/%d\n",
		self.BigPerHost50,
		self.BigPerHost90,
		self.BigPerHostMax,
	)
}

func (self *Density) String() string {
	buf := new(bytes.Buffer)
	self.PrintTo(buf)
	return buf.String()
}

func (self *Density) Clear() {
	self.NonZeroElements = 0
	self.NonZeroHosts = 0
	self.NonZeroPerHost50 = 0
	self.NonZeroPerHost90 = 0
	self.NonZeroPerHostMax = 0

	self.BigElements = 0
	self.BigHosts = 0
	self.BigPerHost50 = 0
	self.BigPerHost90 = 0
	self.BigPerHostMax = 0
}

func (self *Density) CountSparse(s structs.Sparse) {
	self.Clear()

	rowNonZero := make([]int, config.Nhost)
	colNonZero := make([]int, config.Nhost)
	rowBig := make([]int, config.Nhost)
	colBig := make([]int, config.Nhost)

	for col, scol := range s {
		for row, n := range scol {
			if n > 0 {
				self.NonZeroElements++
				colNonZero[col]++
				rowNonZero[row]++
			}
			if n > BigThres {
				self.BigElements++
				colBig[col]++
				rowBig[row]++
			}
		}
	}

	nonZeroMax := 0
	bigMax := 0

	for i := 0; i < config.Nhost; i++ {
		if colNonZero[i]+rowNonZero[i] > 0 {
			self.NonZeroHosts++
		}
		if colBig[i]+rowBig[i] > 0 {
			self.BigHosts++
		}

		if colNonZero[i] > nonZeroMax {
			nonZeroMax = colNonZero[i]
		}
		if rowNonZero[i] > nonZeroMax {
			nonZeroMax = rowNonZero[i]
		}

		if colBig[i] > bigMax {
			bigMax = colBig[i]
		}
		if rowBig[i] > bigMax {
			bigMax = rowBig[i]
		}
	}

	nonZeros := make([]int, 0, config.Nhost*2)
	nonZeros = append(nonZeros, rowNonZero...)
	nonZeros = append(nonZeros, colNonZero...)
	sort.Ints(nonZeros)

	bigs := make([]int, 0, config.Nhost*2)
	bigs = append(bigs, rowBig...)
	bigs = append(bigs, colBig...)
	sort.Ints(bigs)

	self.NonZeroPerHostMax = nonZeroMax
	self.BigPerHostMax = bigMax

	self.NonZeroPerHost50 = nonZeros[config.Nhost]
	self.NonZeroPerHost90 = nonZeros[config.Nhost*2*9/10]
	self.BigPerHost50 = bigs[config.Nhost]
	self.BigPerHost90 = bigs[config.Nhost*2*9/10]
}

func (self *Density) Count(m structs.Matrix) {
	self.Clear()

	nonZeros := make([]int, 0, config.Nhost*2)
	bigs := make([]int, 0, config.Nhost*2)

	for i := 0; i < config.Nlane; i++ {
		if m[i] > 0 {
			self.NonZeroElements++
		}
		if m[i] > BigThres {
			self.BigElements++
		}
	}

	for i := 0; i < config.Nhost; i++ {
		// rowSum := uint64(0)
		// colSum := uint64(0)
		rowNonZero := 0
		colNonZero := 0
		rowBig := 0
		colBig := 0
		for j := 0; j < config.Nhost; j++ {
			l1 := i*config.Nhost + j
			l2 := j*config.Nhost + i
			r := m[l1]
			c := m[l2]
			// rowSum += r
			// colSum += c

			if r > 0 {
				rowNonZero++
			}
			if r > BigThres {
				rowBig++
			}
			if c > 0 {
				colNonZero++
			}
			if c > BigThres {
				colBig++
			}
		}

		/*
			sum := rowSum + colSum
			if sum > 0 {
				self.NonZeroHosts++
			}
			if sum > BigThres*2 {
				self.BigHosts++
			}
		*/

		if rowNonZero > self.NonZeroPerHostMax {
			self.NonZeroPerHostMax = rowNonZero
		}
		if colNonZero > self.NonZeroPerHostMax {
			self.NonZeroPerHostMax = colNonZero
		}
		nonZeros = append(nonZeros, rowNonZero)
		nonZeros = append(nonZeros, colNonZero)

		if rowBig > self.BigPerHostMax {
			self.BigPerHostMax = rowBig
		}
		if colBig > self.BigPerHostMax {
			self.BigPerHostMax = colBig
		}

		bigs = append(bigs, rowBig)
		bigs = append(bigs, colBig)

		if rowNonZero > 0 || colNonZero > 0 {
			self.NonZeroHosts++
		}

		if rowBig > 0 || colBig > 0 {
			self.BigHosts++
		}
	}

	sort.Ints(nonZeros)
	sort.Ints(bigs)

	self.NonZeroPerHost50 = nonZeros[config.Nhost]
	self.NonZeroPerHost90 = nonZeros[config.Nhost*2*9/10]
	self.BigPerHost50 = bigs[config.Nhost]
	self.BigPerHost90 = bigs[config.Nhost*2*9/10]
}

func (self *Density) Max(d *Density) {
	if self.NonZeroElements < d.NonZeroElements {
		self.NonZeroElements = d.NonZeroElements
	}
	if self.NonZeroHosts < d.NonZeroHosts {
		self.NonZeroHosts = d.NonZeroHosts
	}
	if self.NonZeroPerHostMax < d.NonZeroPerHostMax {
		self.NonZeroPerHostMax = d.NonZeroPerHostMax
		self.NonZeroPerHost90 = d.NonZeroPerHost90
		self.NonZeroPerHost50 = d.NonZeroPerHost50
	}

	if self.BigElements < d.BigElements {
		self.BigElements = d.BigElements
	}
	if self.BigHosts < d.BigHosts {
		self.BigHosts = d.BigHosts
	}
	if self.BigPerHostMax < d.BigPerHostMax {
		self.BigPerHostMax = d.BigPerHostMax
		self.BigPerHost90 = d.BigPerHost90
		self.BigPerHost50 = d.BigPerHost50
	}
}
