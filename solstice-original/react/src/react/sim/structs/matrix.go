package structs

import (
	"bytes"
	"fmt"
	"math"
	. "react/sim/config"
)

type Matrix []uint64

func NewMatrix() Matrix {
	return make([]uint64, Nlane)
}

func NewMatrixOf(i uint64) Matrix {
	ret := NewMatrix()
	ret.Assign(i)
	return ret
}

func (m Matrix) Clone() Matrix {
	ret := NewMatrix()
	copy(ret, m)
	return ret
}

func (m Matrix) Set(row, col int, i uint64) {
	assert(row >= 0 && row < Nhost)
	assert(col >= 0 && col < Nhost)
	index := row*Nhost + col
	m[index] = i
}

func (m Matrix) String() string {
	buf := new(bytes.Buffer)
	fmt.Fprint(buf, "[")
	for i := 0; i < Nlane; i++ {
		if i > 0 {
			if i%Nhost == 0 {
				fmt.Fprint(buf, ";")
			}
			fmt.Fprint(buf, " ")
		}
		fmt.Fprintf(buf, "%d", m[i])
	}
	fmt.Fprint(buf, "]")
	return buf.String()
}

func (m Matrix) Marshal() string {
	buf := new(bytes.Buffer)
	for i := 0; i < Nlane; i++ {
		if i > 0 {
			fmt.Fprint(buf, " ")
		}
		fmt.Fprintf(buf, "%d", m[i])
	}
	return buf.String()
}

func (m Matrix) Madd(m2 Matrix) {
	for i := 0; i < Nlane; i++ {
		m[i] += m2[i]
	}
}

func (m Matrix) MapAdd(pm PortMap, a uint64) {
	for row, col := range pm {
		if col < 0 {
			continue
		}
		m[Lane(row, col)] += a
	}
}

func (m Matrix) MapInc(pm PortMap) {
	m.MapAdd(pm, 1)
}

func (m Matrix) Msub(m2 Matrix) {
	for i := 0; i < Nlane; i++ {
		m[i] -= m2[i]
	}
}

func (m Matrix) MnolessThan(m2 Matrix) bool {
	for i := 0; i < Nlane; i++ {
		if m[i] < m2[i] {
			return false
		}
	}
	return true
}

func (m Matrix) Mmul(m2 Matrix) {
	for i := 0; i < Nlane; i++ {
		m[i] *= m2[i]
	}
}

func (m Matrix) Mdiv(m2 Matrix) {
	for i := 0; i < Nlane; i++ {
		if m2[i] == 0 {
			assert(m[i] == 0)
			continue
		}

		m[i] /= m2[i]
	}
}

func (m Matrix) MdivUp(m2 Matrix) {
	for i := 0; i < Nlane; i++ {
		if m2[i] == 0 {
			// assert(m[i] == 0)
			m[i] = 0
			continue
		}

		m[i] += (m2[i] - 1)
		m[i] /= m2[i]
	}
}

func (m Matrix) Clear() {
	m.Assign(0)
}

func (m Matrix) Add(a uint64) {
	for i := 0; i < Nlane; i++ {
		m[i] += a
	}
}

func (m Matrix) NonZeroAdd(a uint64) {
	for i := 0; i < Nlane; i++ {
		if m[i] == 0 {
			continue
		}
		m[i] += a
	}
}

func (m Matrix) Sub(a uint64) {
	for i := 0; i < Nlane; i++ {
		m[i] -= a
	}
}

func (m Matrix) Div(a uint64) Matrix {
	for i := 0; i < Nlane; i++ {
		m[i] /= a
	}
	return m
}

func (m Matrix) RoundDiv(a uint64) {
	m.Round(a)
	m.Div(a)
}

func (m Matrix) Mul(a uint64) {
	for i := 0; i < Nlane; i++ {
		m[i] *= a
	}
}

func (m Matrix) Floor(a uint64) {
	assert(a > 0)
	for i := 0; i < Nlane; i++ {
		m[i] -= m[i] % a
	}
}

func (m Matrix) Ceil(a uint64) {
	assert(a > 0)
	for i := 0; i < Nlane; i++ {
		mod := m[i] % a
		if mod != 0 {
			m[i] += a - mod
		}
	}
}

func (m Matrix) Round(a uint64) {
	m.Add(a / 2)
	m.Floor(a)
}

func (m Matrix) Assign(a uint64) {
	for i := 0; i < Nlane; i++ {
		m[i] = a
	}
}

func (m Matrix) Massign(m2 Matrix) {
	for i := 0; i < Nlane; i++ {
		m[i] = m2[i]
	}
}

func (m Matrix) Mmin(m2 Matrix) {
	for i := 0; i < Nlane; i++ {
		if m2[i] < m[i] {
			m[i] = m2[i]
		}
	}
}

func (m Matrix) Mmax(m2 Matrix) {
	for i := 0; i < Nlane; i++ {
		if m2[i] > m[i] {
			m[i] = m2[i]
		}
	}
}

func (m Matrix) Min(a uint64) {
	for i := 0; i < Nlane; i++ {
		if m[i] > a {
			m[i] = a
		}
	}
}

func (m Matrix) Max(a uint64) {
	for i := 0; i < Nlane; i++ {
		if a > m[i] {
			m[i] = a
		}
	}
}

func (m Matrix) MatLeast(m2 Matrix) {
	m.Mmax(m2)
}

func (m Matrix) MatMost(m2 Matrix) {
	m.Mmin(m2)
}

func (m Matrix) AtLeast(a uint64) {
	m.Max(a)
}

func (m Matrix) NonZeroAtLeast(a uint64) {
	for i := 0; i < Nlane; i++ {
		if m[i] == 0 {
			continue
		}
		if m[i] < a {
			m[i] = a
		}
	}
}

func (m Matrix) AtMost(a uint64) {
	m.Min(a)
}

func (m Matrix) Equals(m2 Matrix) bool {
	for i := 0; i < Nlane; i++ {
		if m[i] != m2[i] {
			return false
		}
	}
	return true
}

func (m Matrix) MinInMap(portMap PortMap) uint64 {
	ret := uint64(math.MaxUint64)
	nvalid := 0

	for row, col := range portMap {
		if col == InvalidPort {
			continue
		}

		nvalid++
		d := m[Lane(row, col)]
		if d < ret {
			ret = d
		}
	}

	if nvalid == 0 {
		return 0
	}

	return ret
}

func (m Matrix) AddInMap(portMap PortMap, a uint64) {
	for i := 0; i < Nhost; i++ {
		if portMap[i] == InvalidPort {
			continue
		}

		m[i*Nhost+portMap[i]] += a
	}
}

func (m Matrix) SubInMap(portMap PortMap, a uint64) {
	for i := 0; i < Nhost; i++ {
		if portMap[i] == InvalidPort {
			continue
		}
		lane := i*Nhost + portMap[i]

		if m[lane] <= a {
			m[lane] = 0
		} else {
			m[lane] -= a
		}
	}
}

func (m Matrix) TrimUnder(a uint64) {
	for i := 0; i < Nlane; i++ {
		if m[i] <= a {
			m[i] = 0
		}
	}
}

func (m Matrix) RowSum(v Vector) {
	v.Clear()
	for i := 0; i < Nhost; i++ {
		for j := 0; j < Nhost; j++ {
			v[i] += m[i*Nhost+j]
		}
	}
}

func (m Matrix) ColSum(v Vector) {
	v.Clear()
	for i := 0; i < Nhost; i++ {
		for j := 0; j < Nhost; j++ {
			v[j] += m[i*Nhost+j]
		}
	}
}

func (m Matrix) MaxLineSum() uint64 {
	return m.Sums(nil, nil)
}

func (m Matrix) Sums(rowSum, colSum Vector) uint64 {
	if rowSum == nil {
		rowSum = NewVector()
	}
	if colSum == nil {
		colSum = NewVector()
	}
	m.RowSum(rowSum)
	m.ColSum(colSum)

	rowMax := rowSum.Max()
	colMax := colSum.Max()
	if colMax > rowMax {
		return colMax
	}
	return rowMax
}

func (m Matrix) Sum() uint64 {
	ret := uint64(0)
	for i := 0; i < Nlane; i++ {
		ret += m[i]
	}

	return ret
}

func (m Matrix) Empty() bool {
	for i := 0; i < Nlane; i++ {
		if m[i] > 0 {
			return false
		}
	}
	return true
}

func (m Matrix) Diameter() int {
	ret := 0
	for i := 0; i < Nhost; i++ {
		for j := 0; j < Nhost; j++ {
			if m[i*Nhost+j] > 0 {
				if i > ret {
					ret = i
				}
				if j > ret {
					ret = j
				}
			}
		}
	}

	return ret + 1
}

func (m Matrix) NormUp(norm uint64, rowBuf, colBuf Vector) {
	sum := m.Sums(rowBuf, colBuf)
	if sum == 0 {
		return
	}
	if sum >= norm {
		return
	}

	m.Mul(norm)
	m.Div(sum)
}

func (m Matrix) Binary() {
	m.AtMost(1)
}

func (m Matrix) SparseStr() string {
	buf := new(bytes.Buffer)
	first := true

	for i := 0; i < Nhost; i++ {
		base := i * Nhost
		for j := 0; j < Nhost; j++ {
			lane := base + j
			a := m[lane]
			if a > 0 {
				if !first {
					fmt.Fprintf(buf, "  ")
				} else {
					first = false
				}
				fmt.Fprintf(buf, "%d-%d: %d", i, j, a)
			}
		}
	}

	return buf.String()
}

func (m Matrix) BinaryStr() string {
	buf := new(bytes.Buffer)
	first := true

	for i := 0; i < Nhost; i++ {
		base := i * Nhost
		for j := 0; j < Nhost; j++ {
			lane := base + j
			a := m[lane]
			if a > 0 {
				if !first {
					fmt.Fprintf(buf, "  ")
				} else {
					first = false
				}
				fmt.Fprintf(buf, "%d-%d", i, j)
			}
		}
	}

	return buf.String()
}

func (m Matrix) MatrixStr() string {
	n := 0
	for i := 0; i < Nlane; i++ {
		thisN := len(fmt.Sprintf("%d", m[i]))
		if thisN > n {
			n = thisN
		}
	}
	f := fmt.Sprintf("%%%dd", n)
	fs := fmt.Sprintf("%%%ds", n)

	buf := new(bytes.Buffer)
	for i := 0; i < Nhost; i++ {
		base := i * Nhost
		for j := 0; j < Nhost; j++ {
			lane := base + j
			if j > 0 {
				fmt.Fprintf(buf, " ")
			}
			v := m[lane]
			if v == 0 {
				fmt.Fprintf(buf, fs, ".")
			} else {
				fmt.Fprintf(buf, f, v)
			}
		}
		fmt.Fprintln(buf)
	}

	return buf.String()
}

func (m Matrix) NormDown(norm uint64, buf Vector) {
	sums := buf
	m.RowSum(sums)

	for i := 0; i < Nhost; i++ {
		j := 0
		s := sums[i]
		for s > norm {
			if m[i*Nhost+j] > 0 {
				m[i*Nhost+j]--
				s--
			}
			j = (j + 1) % Nhost
		}
	}

	m.ColSum(sums)
	for i := 0; i < Nhost; i++ {
		j := 0
		s := sums[i]
		for s > norm {
			if m[j*Nhost+i] > 0 {
				m[j*Nhost+i]--
				s--
			}
			j = (j + 1) % Nhost
		}
	}
}

func (m Matrix) IsPortMap() bool {
	v1 := NewVector()
	v2 := NewVector()
	m.Sums(v1, v2)

	for i := 0; i < Nhost; i++ {
		if v1[i] != 1 {
			return false
		}
	}

	for i := 0; i < Nhost; i++ {
		if v2[i] != 1 {
			return false
		}
	}

	return true
}

func (m Matrix) ToPortMap() PortMap {
	assert(m.IsPortMap())

	ret := NewPortMap()

	for i := 0; i < Nhost; i++ {
		for j := 0; j < Nhost; j++ {
			lane := Lane(i, j)
			if m[lane] > 0 {
				ret[i] = j
				break
			}
		}
	}

	return ret
}
