package replay

import (
	"fmt"
	"io"
)

type Recorder struct {
	w         io.Writer
	nhost     int
	chanNames []string
	t         uint64
}

func NewRecorder(w io.Writer, h *Header) (*Recorder, error) {
	e := h.PrintTo(w)
	if e != nil {
		return nil, e
	}

	ret := new(Recorder)
	ret.w = w
	ret.nhost = h.Nhost
	ret.chanNames = h.ChanNames
	ret.t = 0

	return ret, nil
}

func (self *Recorder) Record(tick *Tick) error {
	_, e := fmt.Fprintln(self.w)
	if e != nil {
		return e
	}

	e = tick.PrintTo(self.w, self.t, self.chanNames, self.nhost)
	self.t++
	return e
}
