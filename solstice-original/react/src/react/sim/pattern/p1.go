package pattern

import (
	"math/rand"

	dem "react/demand"
	. "react/sim/config"
	"react/sim/drainer"
	. "react/sim/structs"
)

// TODO: find a better pattern name
type P1 struct {
	Name string
	Memo string

	N     uint64 // the time length
	Bandw uint64 // bandwidth to fill

	Small      int // number of small bursts
	SmallBandw uint64
	SmallLen   uint64

	Big       int // number of active blows at any time
	BigPeriod uint64
}

func (p *P1) Make() *dem.Demand {
	timeline := NewTimeline(p.N)
	r := rand.New(rand.NewSource(0))
	p.fillSmall(timeline, r)
	p.fillBig(timeline, r)

	d := dem.NewDemand(Nhost)
	d.Name = p.Name
	d.Memo = p.Memo

	timeline.AppendTo(d)

	return d
}

func randLane(r *rand.Rand) (int, int) {
	if Nhost <= 1 {
		panic("nhost too small for randLane")
	}

	row := r.Intn(Nhost)
	col := (row + 1 + r.Intn(Nhost-1)) % Nhost
	return row, col
}

func (p *P1) fillSmall(timeline Timeline, r *rand.Rand) {
	smallLen := p.SmallLen
	if smallLen > p.N {
		smallLen = p.N
	}

	startMax := p.N - smallLen + 1

	for i := 0; i < p.Small; i++ {
		start := uint64(r.Int63n(int64(startMax)))
		end := start + p.SmallLen
		row, col := randLane(r)
		for t := start; t < end; t++ {
			toAdd := timeline[t].ToAdd(row, col, p.Bandw, p.SmallBandw)
			timeline[t].Add(row, col, toAdd)
		}
	}
}

// Repicks p.Big flows. When picked, it filles budget with p.Bandw
// and adds 1 into count
func (p *P1) repickBig(budget Matrix, count Matrix, r *rand.Rand) {
	budget.Clear()
	count.Clear()
	for i := 0; i < p.Big; i++ {
		row, col := randLane(r)
		lane := row*Nhost + col
		budget[lane] = p.Bandw
		count[lane]++
	}
}

func (p *P1) fillBig(timeline Timeline, r *rand.Rand) {
	cross := drainer.NewCrossLimits(p.Bandw)
	d := drainer.NewDrainer(cross.Limits)

	tperiod := uint64(0)
	budget := NewMatrix()
	count := NewMatrix()

	for _, tick := range timeline {
		if tperiod == 0 {
			p.repickBig(budget, count, r)
		}

		cross.LeftOver(tick.D)
		tick.Madd(d.WeightDrain(budget, count))

		tperiod++
		if tperiod >= p.BigPeriod {
			tperiod = 0
		}
	}
}
