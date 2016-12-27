package web

import (
	"encoding/json"
	"fmt"
	"log"
	"net"
	"net/http"
	"time"

	"react/replay"
)

type Server struct {
	rep1  *replay.Replay
	rep2  *replay.Replay
	slice *Slice
}

func NewServer(rep1 *replay.Replay, rep2 *replay.Replay) *Server {
	ret := new(Server)
	ret.rep1 = rep1
	ret.rep2 = rep2

	return ret
}

type header struct {
	Nhost int
	Norm  uint64
}

func (self *Server) writeSlice(w http.ResponseWriter, rep *replay.Replay) {
	chanIndex := rep.QueryChan(self.slice.Chan)
	if chanIndex < 0 {
		log.Fatal(fmt.Errorf("chan %s not found", self.slice.Chan))
	}

	adder := &replay.Adder{
		Chan:   chanIndex,
		Start:  self.slice.Start,
		Window: self.slice.TickPerBar,
	}

	h := &header{
		Nhost: rep.Nhost(),
		Norm:  rep.LinkBw * self.slice.TickPerBar,
	}

	buf, e := json.Marshal(h)
	if e != nil {
		log.Println(e)
		return
	}

	_, e = fmt.Fprintln(w, string(buf))
	if e != nil {
		log.Println(e)
		return
	}

	for i := 0; i < self.slice.Nbar; i++ {
		sum := adder.Add(rep)
		_, e = fmt.Fprintln(w, sum.Json())
		if e != nil {
			log.Println(e)
			return
		}
	}
}

func (self *Server) ServeHTTP(w http.ResponseWriter, r *http.Request) {
	switch r.URL.Path {
	case "/slice1":
		self.writeSlice(w, self.rep1)
	case "/slice2":
		self.writeSlice(w, self.rep2)
	}
}

func (self *Server) Serve(addr, root string, slice *Slice) {
	self.slice = slice

	h := http.NewServeMux()
	h.Handle("/", http.FileServer(http.Dir(root)))
	h.Handle("/slice1", self)
	h.Handle("/slice2", self)

	lis, err := net.Listen("tcp", addr)
	if err != nil {
		log.Fatalln(err)
	}

	server := &http.Server{
		Addr:        addr,
		Handler:     h,
		ReadTimeout: time.Second,
	}

	for {
		err := server.Serve(lis)
		if err != nil {
			log.Println(err)
		}
	}
}
