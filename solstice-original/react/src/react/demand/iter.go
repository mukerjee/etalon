package demand

type Iter struct {
	d *Demand
	i int
	t uint64
	p *Period
	m []Vector
}

func (d *Demand) Iter() *Iter {
	ret := new(Iter)
	ret.d = d

	ret.update()

	return ret
}

func (d *Demand) Chan() <-chan []Vector {
	it := d.Iter()
	ret := make(chan []Vector)

	go func() {
		for it.Has() {
			v := it.Read()
			ret <- v
		}
		close(ret)
	}()

	return ret
}

func (d *Demand) WindChan(windSize uint64) <-chan []Vector {
	assert(windSize > 0)

	ret := make(chan []Vector)

	go func() {
		sum := newMatrix(d.nhost)
		n := uint64(0)
		c := d.Chan()
		for m := range c {
			// add into sum
			for row, v := range sum {
				for col := range v {
					v[col] += m[row][col]
				}
			}

			// update counter
			n++
			if n >= windSize {
				assert(n == windSize)
				n = 0
				ret <- sum
				sum = newMatrix(d.nhost)
			}
		}

		if n > 0 {
			ret <- sum
		}

		close(ret)
	}()

	return ret
}

func (it *Iter) closed() bool {
	return it.i >= len(it.d.periods)
}

func (it *Iter) update() {
	if it.closed() {
		it.p = nil
		it.m = nil
	}

	it.p = it.d.periods[it.i]
	for it.t >= it.p.N {
		it.t -= it.p.N
		it.i++
		if it.closed() {
			it.p = nil
			it.m = nil
			return
		}

		it.p = it.d.periods[it.i]
	}

	it.m = it.p.D
}

func (it *Iter) next() {
	if it.closed() {
		return
	}

	it.t++
	it.update()
}

func (it *Iter) Has() bool {
	return !it.closed()
}

func (it *Iter) Read() []Vector {
	ret := it.m
	it.next()
	return ret
}
