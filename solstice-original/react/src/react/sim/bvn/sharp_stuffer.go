package bvn

import (
	"math/rand"
	"sort"

	. "react/sim/config"
	. "react/sim/structs"
)

type SharpStuffer struct {
	rowSum, colSum Vector
	rowVec, colVec sumVector

	random *rand.Rand
}

func NewSharpStuffer() *SharpStuffer {
	ret := new(SharpStuffer)
	ret.rowSum = NewVector()
	ret.colSum = NewVector()
	ret.rowVec = newSumVector()
	ret.colVec = newSumVector()

	ret.random = rand.New(rand.NewSource(372))

	return ret
}

var _ Stuffer = new(SharpStuffer)
var _ SparseStuffer = new(SharpStuffer)

type sumItem struct {
	id  int
	sum uint64
}

type sumVector []*sumItem

func newSumVector() sumVector {
	ret := make([]*sumItem, Nhost)
	for i := range ret {
		ret[i] = new(sumItem)
		ret[i].id = i
	}
	return ret
}

func (v sumVector) Reset(vec Vector) {
	for i, a := range vec {
		v[i].id = i
		v[i].sum = a
	}
}

func (v sumVector) Len() int           { return len(v) }
func (v sumVector) Less(i, j int) bool { return v[i].sum < v[j].sum }
func (v sumVector) Swap(i, j int)      { v[i], v[j] = v[j], v[i] }

func (self *SharpStuffer) Stuff(m Matrix, norm uint64) uint64 {
	return StuffWithSparse(self, m, norm)
}

func (self *SharpStuffer) StuffSparse(sparse Sparse, norm uint64) uint64 {
	rowSum := self.rowSum
	colSum := self.colSum

	maxSum := sparse.Sums(rowSum, colSum)
	if norm < maxSum {
		norm = maxSum
	}

	// we stuff non-zero data first
	colPerm := self.random.Perm(len(sparse))

	for _, col := range colPerm {
		scol := sparse[col]

		var rows []int
		for row := range scol {
			rows = append(rows, row)
		}
		sort.Ints(rows)

		perm := self.random.Perm(len(rows))

		for _, i := range perm {
			row := rows[i]

			rowDelta := norm - rowSum[row]
			colDelta := norm - colSum[col]
			delta := rowDelta
			if delta > colDelta {
				delta = colDelta
			}

			if delta > 0 {
				scol[row] += delta
				rowSum[row] += delta
				colSum[col] += delta
			}
		}
	}

	// after this round, it should be guaranteed that for each non-zero
	// element, either its row or its column is stuffed. i.e. there is no way
	// to stuffe more over a non-zero element

	rowVec := self.rowVec
	colVec := self.colVec
	rowVec.Reset(rowSum)
	colVec.Reset(colSum)
	sort.Sort(rowVec)
	sort.Sort(colVec)

	rowIter := 0
	colIter := 0
out:
	for {
		for rowVec[rowIter].sum >= norm {
			assert(rowVec[rowIter].sum == norm)
			rowIter++
			if rowIter >= Nhost {
				break out
			}
		}

		for colVec[colIter].sum >= norm {
			assert(colVec[colIter].sum == norm)
			colIter++
			assert(colIter < Nhost)
		}

		rowDelta := norm - rowVec[rowIter].sum
		colDelta := norm - colVec[colIter].sum
		delta := rowDelta
		if delta > colDelta {
			delta = colDelta
		}

		if delta > 0 {
			// println(row, col, sparse.At(row, col))
			row := rowVec[rowIter].id
			col := colVec[colIter].id
			// assert(sparse.At(row, col) == 0)
			sparse.SetAt(row, col, sparse.At(row, col)+delta)
			rowSum[row] += delta
			colSum[col] += delta
			rowVec[rowIter].sum += delta
			colVec[colIter].sum += delta
		}
	}

	// and we are done here
	return norm
}
