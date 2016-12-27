package headq

import (
	"fmt"
	"io"
	"log"
	"net"
	"os"
	"time"

	"fprox"
	"react/demand"
	"react/replay"
	tbdem "react/testb/demand"
	"react/testb/setup"
)

type Headq struct {
	lgs []*fprox.Client
}

func NewHeadq() *Headq {
	return NewHeadqHosts(setup.FullTestbed())
}
func NewHeadqLittleHalf() *Headq {
	return NewHeadqHosts(setup.LittleHalf())
}
func NewHeadqBigHalf() *Headq {
	return NewHeadqHosts(setup.BigHalf())
}

func NewHeadqHosts(hosts []int) *Headq {
	n := setup.Nhost
	ret := new(Headq)
	ret.lgs = make([]*fprox.Client, n)

	var e error
	for _, i := range hosts {
		if i < 0 || i >= n {
			log.Printf("warning: host %d is out of range", i)
			continue
		}

		addr := fmt.Sprintf("%s:%d", setup.LocalIp(i), setup.LoadgPort)
		ret.lgs[i], e = fprox.NewClient(addr)
		ret.noError(i, e)
		ret.lgs[i].Name = fmt.Sprintf("loadg%d", i)
	}

	return ret
}

func (self *Headq) noError(i int, e error) {
	if e != nil {
		log.Fatalln(fmt.Errorf("loadg%d: %s", i, e))
	}
}

func (self *Headq) SetVerbose(v bool) {
	for _, lg := range self.lgs {
		if lg == nil {
			continue
		}
		lg.Verbose = true
	}
}

func (self *Headq) clearSink() {
	for i, lg := range self.lgs {
		if lg == nil {
			continue
		}
		e := lg.Write("recv", "clear\n")
		self.noError(i, e)
	}
}

func (self *Headq) TestDemand() {
	d := tbdem.New()
	for i := 0; i < 5; i++ {
		d.Add(tbdem.P4r(5000, 1000, 50, 50))
		d.Add(tbdem.P4r(5000, 500, 500, 50))
		d.Add(tbdem.P4r(5000, 334, 334, 334))
		d.Add(tbdem.P0(5000))
	}

	self.Demand(d)
}

func (self *Headq) Demand(d *demand.Demand) {
	for i, lg := range self.lgs {
		if lg == nil {
			continue
		}
		f, e := lg.Create("demand")
		self.noError(i, e)

		e = tbdem.WriteHost(f, d, i)
		self.noError(i, e)

		f.Close()
	}
}

func (self *Headq) start(i int, lg *fprox.Client, c chan<- int) {
	e := lg.Write("pgctrl", "start\n")
	self.noError(i, e)
	c <- 1
}

type Runnings <-chan int

func (self *Headq) Start() Runnings {
	self.clearSink()

	c := make(chan int)
	for i, lg := range self.lgs {
		if lg == nil {
			continue
		}
		go self.start(i, lg, c)
	}

	return Runnings(c)
}

func (self *Headq) Join(r Runnings) {
	for i, lg := range self.lgs {
		if lg == nil {
			continue
		}
		<-r
		log.Printf("loadg%d joined", i)
	}
}

func (self *Headq) DumpSend() Dumper {
	ret := NewTimeDumper()

	for i, lg := range self.lgs {
		if lg == nil {
			continue
		}

		f, e := lg.Open("send")
		self.noError(i, e)
		ret.Add(i, NewChanDumper(f, i))
	}

	return newSource(ret)
}

func (self *Headq) DumpRecv() Dumper {
	ret := NewTimeDumper()

	for i, lg := range self.lgs {
		if lg == nil {
			continue
		}

		f, e := lg.Open("recv")
		self.noError(i, e)
		ret.Add(i, NewChanDumper(f, i))
	}

	return newSink(ret)
}

func (self *Headq) SaveReplay(path string) error {
	fout, e := os.Create(path)
	if e != nil {
		return e
	}
	defer fout.Close()

	return self.WriteReplayTo(fout)
}

// saves replay to the writer, return any io error got on writing
func (self *Headq) WriteReplayTo(out io.Writer) error {
	header := &replay.Header{
		Nhost:  setup.Nhost,
		LinkBw: setup.LinkBw,
		PackBw: setup.PackBw,
		ChanNames: []string{
			"send",
			"recv",
		},
	}
	rec, e := replay.NewRecorder(out, header)
	if e != nil {
		return e
	}

	ticker := NewTicker(self.DumpSend(), self.DumpRecv())

	for ticker.Check() {
		rec.Record(ticker.Tick())
	}

	return nil
}

func Beat(addr string) {
	log.Println("(beat)")

	_c, e := net.DialTimeout("tcp4", addr, time.Second)
	noError(e)

	c := _c.(*net.TCPConn)

	_, e = c.Write([]byte("beat\n"))
	noError(e)

	c.Close()
}
