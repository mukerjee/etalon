// Package loadgen provides a server that controls the loadgen module
package loadg

import (
	"bufio"
	"errors"
	"fmt"
	"fprox"
	"net"
	"os"

	"react/testb/env"
	"react/testb/setup"
)

const (
	procDir   = "/proc/net/loadgen/"
	defthread = "kpktgend_3"
)

func proc(s string) string { return procDir + s }

type Loadgen struct {
	*fprox.Server

	thread string
	iface  string

	conn   *net.TCPConn
	reader *bufio.Reader

	err   error
	njson int
}

func listenTCP(addr string) (*net.TCPListener, error) {
	a, e := net.ResolveTCPAddr("tcp4", addr)
	if e != nil {
		return nil, e
	}

	listener, e := net.ListenTCP("tcp4", a)
	if e != nil {
		return nil, e
	}

	return listener, nil
}

func NewLoadgen(addr string) (*Loadgen, error) {
	if !findModule("loadgen") {
		return nil, errors.New("loadgen module not found")
	}

	ret := new(Loadgen)
	ret.thread = defthread
	ret.iface = env.Iface
	ret.setup()

	ret.Server = &fprox.Server{
		Addr:   addr,
		Mapper: ret,
	}

	return ret, nil
}

func lg(dev, s string) {
	logf(": %s > %s", s, dev)

	f, e := os.Create(proc(dev))
	noError(e)

	_, e = fmt.Fprintln(f, s)
	noError(e)

	noError(f.Close())
}

func (self *Loadgen) setup() {
	if self.iface == "" {
		panic(errors.New("dev missing"))
	}

	lg(self.thread, "rem_device_all")
	lg(self.thread, "add_device "+self.iface)

	lg(self.iface, fmt.Sprintf("pkt_size %d", setup.PackSize))
	lg(self.iface, "dst 192.168.1.1")
	lg(self.iface, "clone_skb 1")
	lg(self.iface, "count 0")
	lg(self.iface, "delay 0")
	lg(self.iface, "flag REACTOR")

	lg("sink", self.iface)
}

func (self *Loadgen) MapRead(path string) (string, error) {
	logf("read %s", path)

	switch path {
	case "send":
		return proc(self.iface + ".dmp"), nil
	case "recv":
		return proc("sink.dmp"), nil
	}

	return "", errors.New("invalid read path")
}

func (self *Loadgen) MapWrite(path string) (string, error) {
	logf("write %s", path)

	switch path {
	case "demand":
		return proc(self.iface + ".dem"), nil
	case "thread":
		return proc(self.thread), nil
	case "iface":
		return proc(self.iface), nil
	case "pgctrl":
		return proc("pgctrl"), nil
	case "recv":
		return proc("sink.dmp"), nil
	}

	return "", errors.New("invalid write path")
}
