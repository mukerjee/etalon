package csol

// #include "sols.h"
import "C"

import (
	// "fmt"
	"runtime"

	"react/sim"
	"react/sim/config"
	. "react/sim/structs"
)

type Scheduler struct {
	sols C.sols_t

	demand Matrix
	bandw  Matrix
}

func New() *Scheduler {
	ret := new(Scheduler)
	C.sols_init(&ret.sols, C.int(config.Nhost))

	ret.sols.night_len = C.uint64_t(config.NightLen)
	ret.sols.week_len = C.uint64_t(config.WeekLen)
	ret.sols.min_day_len = C.uint64_t(config.MinDayLen)
	ret.sols.avg_day_len = C.uint64_t(config.AvgDayLen)
	ret.sols.day_len_align = C.uint64_t(config.TickPerGrid)
	ret.sols.pack_bw = C.uint64_t(config.PackBw)
	ret.sols.link_bw = C.uint64_t(config.LinkBw)

	runtime.SetFinalizer(ret, cleanup)

	ret.demand = NewMatrix()
	ret.bandw = NewMatrix()

	return ret
}

func (s *Scheduler) normalize(d Matrix, w uint64) {
	if w == config.WeekLen {
		return
	}
	d.Mul(config.WeekLen)
	d.Div(w)
}

var _ sim.Scheduler = new(Scheduler)

func (s *Scheduler) Schedule(r, d Matrix, w uint64) ([]*Day, Matrix) {
	copy(s.demand, d)
	s.normalize(s.demand, w)

	dem := &s.sols.future
	C.sols_mat_clear(dem)
	// ignore residue for now

	for i := 0; i < config.Nhost; i++ {
		for j := 0; j < config.Nhost; j++ {
			lane := i*config.Nhost + j
			v := s.demand[lane]
			/*
				if i == j {
					continue
				}
			*/
			C.sols_mat_set(dem, C.int(i), C.int(j), C.uint64_t(v))
		}
	}

	C.sols_schedule(&s.sols)

	return s.results()
}

func (s *Scheduler) results() ([]*Day, Matrix) {
	days := make([]*Day, int(s.sols.nday))
	bandw := s.bandw
	bandw.Clear()

	for i := range days {
		d := &s.sols.sched[i]
		m := NewMatrix()

		for dest := 0; dest < config.Nhost; dest++ {
			if int(C.sols_day_is_dummy(d, C.int(dest))) != 0 {
				continue
			}

			src := int(C.sols_day_input_port(d, C.int(dest)))
			lane := src*config.Nhost + dest
			m[lane] = 1
			bandw[lane] = uint64(C.sols_bw_limit(&s.sols, C.int(lane)))
			// bandw[lane] = config.LinkBw - config.PackBw
		}

		days[i] = NewDay(m, uint64(d.len))
	}

	// fmt.Println(bandw.MatrixStr())
	return days, bandw
}

func cleanup(s *Scheduler) {
	C.sols_cleanup(&s.sols)
}
