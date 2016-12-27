package eval

import (
	"bytes"
	"fmt"
	"time"

	"react/sim"
	. "react/sim/config"
	"react/sim/drainer"
	. "react/sim/structs"
)

type Report struct {
	Nday    int
	Ndemand uint64
	Ncap    uint64

	NcircServed        uint64
	NhybridServed      uint64
	NsmartHybridServed uint64

	CircUsed Matrix
	PackUsed Matrix
	Unserved Matrix

	ComputeTime time.Duration
}

func perc(a, t uint64) float64 {
	return float64(a) / float64(t) * 100
}

func (r *Report) String() string {
	ret := new(bytes.Buffer)

	fmt.Fprintf(ret, "%d (%.3f%%) bytes to serve\n",
		r.Ndemand, perc(r.Ndemand, r.Ncap),
	)
	fmt.Fprintf(ret, "  schedule of %d days\n", r.Nday)
	if r.ComputeTime != 0 {
		fmt.Fprintf(ret, "  computed in %s\n", r.ComputeTime)
	}

	fmt.Fprintf(ret, "  %d (%.3f%%, %.3f%%) served on circuit\n",
		r.NcircServed,
		perc(r.NcircServed, r.Ndemand),
		perc(r.NcircServed, r.Ncap),
	)
	fmt.Fprintf(ret, "  %d (%.3f%%, %.3f%%) served on hybrid\n",
		r.NhybridServed,
		perc(r.NhybridServed, r.Ndemand),
		perc(r.NhybridServed, r.Ncap),
	)
	fmt.Fprintf(ret, "  %d (%.3f%%, %.3f%%) served on smart hybrid\n",
		r.NsmartHybridServed,
		perc(r.NsmartHybridServed, r.Ndemand),
		perc(r.NsmartHybridServed, r.Ncap),
	)

	return ret.String()
}

func EvalScheduler(s sim.Scheduler, demand Matrix) *Report {
	m := demand

	orig := m.Clone()
	timeStart := time.Now()

	days, bandw := s.Schedule(nil, m, WeekLen)

	computeTime := time.Since(timeStart)

	r := newReport(orig, days, bandw)
	r.ComputeTime = computeTime
	r.Ncap = LinkBw * WeekLen * uint64(Nhost)
	return r
}

func newReport(m Matrix, days []*Day, bandw Matrix) *Report {
	ret := new(Report)
	ret.Nday = len(days)

	ret.countBytes(m, days, bandw)
	ret.Ndemand = m.Sum()

	return ret
}

func (r *Report) countBytes(m Matrix, days []*Day, bandw Matrix) {
	served := NewMatrix()
	nonCirc := NewMatrix()
	unserved := NewMatrix()

	t := NewMatrix()

	// fmt.Println(bandw.MatrixStr())

	for _, day := range days {
		if day.Len <= NightLen {
			continue
		}

		copy(t, day.Sched)
		t.Mul(day.Len - NightLen)
		t.Mmul(bandw)
		served.Madd(t)
	}

	served.MatMost(m)

	for i, s := range served {
		if s < m[i] {
			unserved[i] = m[i] - s
			if s == 0 {
				nonCirc[i] = m[i]
			}
		}
	}

	cross := drainer.NewCrossLimits(PackBw * WeekLen)
	d := drainer.NewDrainer(cross.Limits)

	r.PackUsed = d.Drain(nonCirc).Clone()
	r.Unserved = unserved.Clone()
	npack1 := r.PackUsed.Sum()
	npack2 := d.Drain(unserved).Sum()
	nbyte := served.Sum()

	r.NcircServed = nbyte
	r.NhybridServed = nbyte + npack1
	r.NsmartHybridServed = nbyte + npack2
}
