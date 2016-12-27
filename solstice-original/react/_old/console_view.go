package react

import (
	"bytes"
	"fmt"
)

const (
	MatrixRows     = 4
	MatrixCols     = 2
	MatrixCount    = MatrixRows * MatrixCols
	InfoHeight     = 1
	NumWidth       = 4
	MatMargin      = 3
	FlowLeftMargin = 1
	FlowWidth      = 35
)

const CSI = "\x1b["

type ConsoleView struct {
	weekLen, dayLen, nightLen uint64
	linkBw, packBw            uint64

	sched map[int]*Slice

	height        int
	matWidth      int
	matHeight     int
	matBlankLine  string
	flowBlankLine string

	flowLeft   int
	flowHeight int

	rowSum, colSum Vector
}

var _ View = new(ConsoleView)

func NewConsoleView() *ConsoleView {
	ret := new(ConsoleView)
	ret.rowSum = NewVector()
	ret.colSum = NewVector()

	return ret
}

func blankString(n int) string {
	buf := new(bytes.Buffer)
	for i := 0; i < n; i++ {
		buf.WriteString(" ")
	}
	return buf.String()
}

func (self *ConsoleView) Reset(n int) {
	assert(nhost == n)

	self.matWidth = NumWidth + 1 + (NumWidth+1)*n
	self.matHeight = n + 2
	self.matBlankLine = blankString(self.matWidth)

	self.height += InfoHeight + (self.matHeight+1)*MatrixRows

	self.flowLeft = (self.matWidth+MatMargin)*MatrixCols + FlowLeftMargin
	self.flowHeight = self.height - InfoHeight
	self.flowBlankLine = blankString(FlowWidth)

	for i := 0; i < self.height; i++ {
		fmt.Println() // scroll down
	}

	fmt.Print(CSI, "s")
	fmt.Print(CSI, "?25l")
}

func (self *ConsoleView) loc(row, col int) {
	fmt.Print(CSI, "u")
	assert(row >= 0 && row < self.height)
	assert(col >= 0)
	up := self.height - row
	if up > 0 {
		fmt.Print(CSI, up, "A")
	}
	if col > 0 {
		fmt.Print(CSI, col, "C")
	}
}

func (self *ConsoleView) locMatrix(id int, row, col int) {
	mrow := id / MatrixCols
	mcol := id % MatrixCols
	row += (self.matHeight+1)*mrow + InfoHeight
	col += (self.matWidth + MatMargin) * mcol

	self.loc(row, col)
}

func (self *ConsoleView) done() {
	fmt.Print(CSI, "u")
}

func (self *ConsoleView) ConfigTime(w, d, n uint64) {
	assert(w == weekLen)
	assert(d == dayLen)
	assert(n == nightLen)
}

func (self *ConsoleView) ConfigBandw(link, pack uint64) {
	assert(link == linkBw)
	assert(pack == packBw)
}

func (self *ConsoleView) Time(t uint64) {
	self.loc(0, 0)
	fmt.Print("t=", t)

	self.done()
}

func (self *ConsoleView) MatrixClear(id int) {
	if id >= MatrixCount {
		return
	}

	for i := 0; i < self.matHeight; i++ {
		self.locMatrix(id, i, 0)
		fmt.Print(self.matBlankLine)
	}

	self.done()
}

func (self *ConsoleView) MatrixTitle(id int, s string) {
	if id >= MatrixCount {
		return
	}

	self.locMatrix(id, 0, 0)
	fmt.Print(self.matBlankLine)
	self.locMatrix(id, 0, 0)
	fmt.Print(CSI, "33;1m")
	fmt.Print("       ", s)
	fmt.Print(CSI, "0m")

	self.done()
}

func (self *ConsoleView) MatrixNorm(id int, one uint64) {
}

func printNum(i uint64) {
	if i == 0 {
		fmt.Print("   .")
	} else {
		fmt.Print(Snum(i))
	}
}

func printBlueNum(i uint64) {
	fmt.Print(CSI, "34m")
	printNum(i)
	fmt.Print(CSI, "0m")
}

func Snum(i uint64) string {
	scales := []struct {
		unit uint64
		name string
	}{
		{uint64(1), ""},
		{uint64(1e3), "k"},
		{uint64(1e6), "m"},
		{uint64(1e9), "g"},
		{uint64(1e12), "t"},
		{uint64(1e15), "p"},
	}

	for _, s := range scales {
		u := s.unit
		if u == 1 {
			if i < 10000 {
				return fmt.Sprintf("%4d", i)
			}
		}

		if i/u < 1000 {
			return fmt.Sprintf("%3d%s", (i+u/2)/u, s.name)
		}
	}

	return "BIG!"
}

func (self *ConsoleView) Matrix(id int, m Matrix) {
	if id >= MatrixCount || m == nil {
		return
	}

	m.sums(self.rowSum, self.colSum)

	for i := 0; i < nhost; i++ {
		for j := 0; j < nhost; j++ {
			self.locMatrix(id, 1+i, (j+1)*(NumWidth+1)+1)
			printNum(m[i*nhost+j])
		}
	}

	for i := 0; i < nhost; i++ {
		self.locMatrix(id, 1+i, 0)
		printBlueNum(self.rowSum[i])
	}

	for i := 0; i < nhost; i++ {
		self.locMatrix(id, 1+nhost, (i+1)*(NumWidth+1)+1)
		printBlueNum(self.colSum[i])
	}

	self.done()
}

func (self *ConsoleView) SchedClearAfter(i int) {
}

func (self *ConsoleView) Sched(i int, day *Day) {
}

func (self *ConsoleView) SchedTime(day int, timeOfDay uint64, isNight bool) {
	self.loc(0, 20)
	fmt.Printf("day=%-4d timeOfDay=%-7d isNight=%v ",
		day, timeOfDay, isNight)

	self.done()
}

func (self *ConsoleView) FlowClearAfter(n int) {
	for i := n; i < self.flowHeight; i++ {
		self.loc(InfoHeight+i, self.flowLeft)
		fmt.Print(self.flowBlankLine)
	}

	self.done()
}

func (self *ConsoleView) Flow(i int, flow *Flow) {
	if i >= self.flowHeight {
		return
	}

	self.loc(InfoHeight+i, self.flowLeft)
	src := flow.lane / nhost
	dest := flow.lane % nhost
	fmt.Printf("%2d>%-2d : %s/%s/%s, %s/t ", src, dest,
		Snum(flow.sent), Snum(flow.acked),
		Snum(flow.size), Snum(flow.rate))
	if flow.stable {
		fmt.Printf(" ")
	} else {
		fmt.Printf("*")
	}

	self.done()
}

func (self *ConsoleView) Message(s string) {
}

func (self *ConsoleView) Close() {
	fmt.Print(CSI, "?25h")
}
