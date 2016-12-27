package web

import (
	"encoding/json"
	"fmt"
	"log"
	"net"
	"net/http"
	"time"

	. "react/sim"
	"react/sim/clock"
	"react/sim/config"
)

type Config struct {
	TickPerHour  uint64
	HourPerFrame int
}

func NewConfig() *Config {
	ret := new(Config)
	ret.TickPerHour = config.TickPerHour
	ret.HourPerFrame = 1

	return ret
}

func (self *Config) String() string {
	s, err := json.Marshal(self)
	noError(err)
	return string(s)
}

func (self *Config) Check() {
	if self.TickPerHour == 0 {
		self.TickPerHour = 1
	}

	if self.HourPerFrame <= 0 {
		self.HourPerFrame = 1
	}
}

func (self *Config) Apply() {
	self.Check()
	config.TickPerHour = self.TickPerHour
}

type Server struct {
	pipe     chan []byte
	restarts chan int

	C *Config
}

type Msg struct {
	T uint64
	C string
	D interface{}
}

func NewServer(addr, root string) (*Server, error) {
	ret := new(Server)
	ret.pipe = make(chan []byte, 10)
	ret.restarts = make(chan int, 5)
	ret.C = NewConfig()

	go ret.Serve(addr, root)

	return ret, nil
}

var _ Display = new(Server)
var _ Stopper = new(Server)

func (self *Server) TakesHour() bool { return true }

func (self *Server) emit(t string, d interface{}) {
	s, err := json.Marshal(&Msg{clock.T, t, d})
	noError(err)

	self.pipe <- s
}

func (self *Server) printAll() {
	for s := range self.pipe {
		fmt.Println(string(s))
	}
}

func (self *Server) DrawHour(r *HourReport) {
	self.emit("hour", r)
}

func (self *Server) Start(setup *Setup) { self.emit("start", setup) }
func (self *Server) Stop()              { self.emit("stop", nil) }

func noError(e error) {
	if e != nil {
		panic(e)
	}
}

func (self *Server) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	switch r.URL.Path {
	case "/frame":
		max := self.C.HourPerFrame
		n := 0
		for len(self.pipe) > 0 && n < max {
			_, err := w.Write(<-self.pipe)
			noError(err)
			fmt.Fprintln(w)
			n++
		}
	case "/restart":
		dec := json.NewDecoder(r.Body)
		err := dec.Decode(self.C)
		noError(err)

		self.restarts <- 1
	}
}

func (self *Server) WaitStart() bool {
	<-self.restarts
	for len(self.restarts) > 0 {
		<-self.restarts
	}

	self.C.Apply()
	return true
}

func (self *Server) ShouldStop() bool {
	return len(self.restarts) > 0
}

func (self *Server) Serve(addr, root string) {
	h := http.NewServeMux()
	h.Handle("/", http.FileServer(http.Dir(root)))
	h.Handle("/frame", self)
	h.Handle("/restart", self)

	lis, err := net.Listen("tcp", addr)
	if err != nil {
		log.Fatalln(err)
	}

	server := new(http.Server)
	server.Addr = addr
	server.Handler = h
	server.ReadTimeout = time.Second

	for {
		err := server.Serve(lis)
		if err != nil {
			log.Println(err)
		}
	}
}
