package main

import (
	"bytes"
	"errors"
	"fmt"
	"os"
	"react/replay"
)

func noError(e error) {
	if e != nil {
		fmt.Fprintln(os.Stderr, e.Error())
		os.Exit(1)
	}
}

func imax(i1, i2 int) int {
	if i1 > i2 {
		return i1
	}
	return i2
}

func udelta(u1, u2 uint64) uint64 {
	if u1 > u2 {
		return u1 - u2
	}
	return u2 - u1
}

type Report struct {
	s1, s2 []uint64 // total sums
	d1, d2 []uint64 // sum of delta absolutes
	n      uint64
	nhost  int
}

func (self *Report) mstring(m []uint64) string {
	buf := new(bytes.Buffer)
	nhost := self.nhost
	sum := uint64(0)
	n := float64(self.n)

	for i := 0; i < nhost; i++ {
		for j := 0; j < nhost; j++ {
			index := i*nhost + j
			a := m[index]
			if a == 0 {
				fmt.Fprintf(buf, " %6s", "-")
			} else {
				f := float64(a) / n
				fmt.Fprintf(buf, " %6.1f", f)
			}
			sum += a
		}
		fmt.Fprintln(buf)
	}
	fmt.Fprintf(buf, "(sum=%.2f)\n", float64(sum)/n)

	return buf.String()
}

func (self *Report) mpercent(m []uint64, base []uint64) string {
	buf := new(bytes.Buffer)

	nhost := self.nhost
	for i := 0; i < nhost; i++ {
		for j := 0; j < nhost; j++ {
			index := i*nhost + j
			a := m[index]
			b := base[index]
			if b == 0 || a == 0 {
				fmt.Fprintf(buf, " %6s", "-")
			} else {
				f := float64(a) / float64(b) * 100
				fmt.Fprintf(buf, " %5.1f%%", f)
			}
		}
		fmt.Fprintln(buf)
	}

	return buf.String()
}

func (self *Report) String() string {
	buf := new(bytes.Buffer)

	fmt.Fprintln(buf, "s1 =")
	fmt.Fprintln(buf, self.mstring(self.s1))

	fmt.Fprintln(buf, "s2 =")
	fmt.Fprintln(buf, self.mstring(self.s2))

	fmt.Fprintln(buf, "d1 =")
	fmt.Fprintln(buf, self.mstring(self.d1))

	fmt.Fprintln(buf, "d2 =")
	fmt.Fprintln(buf, self.mstring(self.d2))

	fmt.Fprintln(buf, "p1 =")
	fmt.Fprintln(buf, self.mpercent(self.d1, self.s1))

	fmt.Fprintln(buf, "p2 =")
	fmt.Fprintln(buf, self.mpercent(self.d2, self.s2))

	return buf.String()
}

func mradd(s []uint64, m replay.Matrix) {
	madd(s, ([]uint64)(m))
}

func madd(m1, m2 []uint64) {
	if len(m1) != len(m2) {
		panic("length mismatch")
	}

	for i, a := range m2 {
		m1[i] += a
	}
}

func mclear(m []uint64) {
	for i := range m {
		m[i] = 0
	}
}

func msum(m []uint64) uint64 {
	ret := uint64(0)
	for _, a := range m {
		ret += a
	}
	return ret
}

func mdelta(delta, m1, m2 []uint64) {
	if len(delta) != len(m1) || len(delta) != len(m2) {
		panic("length mismatch")
	}

	for i := range delta {
		delta[i] = udelta(m1[i], m2[i])
	}
}

func mxdelta(d1, d2, m1, m2 []uint64) {
	if len(m1) != len(m2) || len(m1) != len(d1) || len(m1) != len(d2) {
		panic("length mismatch")
	}

	for i, a1 := range m1 {
		a2 := m2[i]
		if a1 > a2 {
			d1[i] = a1 - a2
			d2[i] = 0
		} else {
			d2[i] = a2 - a1
			d1[i] = 0
		}
	}
}

const (
	step    = uint64(100)
	offset1 = uint64(0)
	offset2 = uint64(0)
)

func diff(rep1, rep2 *replay.Replay, c string) (*Report, error) {
	if rep1.Nhost() != rep2.Nhost() {
		return nil, errors.New("different host count")
	}

	if rep1.LinkBw != rep2.LinkBw || rep1.PackBw != rep2.PackBw {
		return nil, errors.New("different bandwidth config")
	}

	c1 := rep1.QueryChan(c)
	if c1 == -1 {
		return nil, errors.New("channel not found in replay1")
	}

	c2 := rep2.QueryChan(c)
	if c2 == -1 {
		return nil, errors.New("channel not found in replay2")
	}

	n := uint64(imax(rep1.Len(), rep2.Len()))

	nhost := rep1.Nhost()
	nentry := nhost * nhost
	mmake := func() []uint64 {
		return make([]uint64, nentry)
	}

	mstr := func(d []uint64) string {
		buf := new(bytes.Buffer)

		for i := 0; i < nhost; i++ {
			for j := 0; j < nhost; j++ {
				fmt.Fprintf(buf, " %d", d[i*nhost+j])
			}
			fmt.Fprint(buf, ";")
		}

		return buf.String()
	}

	s1 := mmake()
	s2 := mmake()
	d1 := mmake()
	d2 := mmake()

	ds1 := mmake()
	ds2 := mmake()
	ss1 := mmake()
	ss2 := mmake()

	for i := uint64(0); i < n; i += step {
		mclear(s1)
		mclear(s2)

		for j := uint64(0); j < step; j++ {
			t := i + j
			mradd(s1, rep1.Matrix(c1, t+offset1))
			mradd(s2, rep2.Matrix(c2, t+offset2))
		}

		mxdelta(d1, d2, s1, s2)

		madd(ss1, s1)
		madd(ss2, s2)

		madd(ds1, d1)
		madd(ds2, d2)

		fmt.Println("s1:", mstr(s1))
		fmt.Println("s2:", mstr(s2))
		fmt.Println("d1:", mstr(d1))
		fmt.Println("d2:", mstr(d2))
	}

	return &Report{
		s1:    ss1,
		s2:    ss2,
		d1:    ds1,
		d2:    ds2,
		n:     n,
		nhost: rep1.Nhost(),
	}, nil
}

func main() {
	rep1, e := replay.Load("sim.rep")
	noError(e)

	rep2, e := replay.Load("test.rep")
	noError(e)

	repSend, e := diff(rep1, rep2, "send")
	noError(e)

	repRecv, e := diff(rep1, rep2, "recv")
	noError(e)

	nhost := rep1.Nhost()
	fmt.Printf("%d hosts, rep1: %d ticks, rep2: %d ticks\n",
		nhost, rep1.Len(), rep2.Len())

	fmt.Println("[send]")
	fmt.Print(repSend)

	fmt.Println("[recv]")
	fmt.Print(repRecv)
}
