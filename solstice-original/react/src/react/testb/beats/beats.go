package beats

import (
	"bufio"
	"encoding/binary"
	"log"
	"net"
)

func noError(e error) {
	if e != nil {
		log.Fatalln(e)
	}
}

type Beater struct {
	addr1 *net.UDPAddr
	addr2 *net.UDPAddr

	conn *net.UDPConn
}

const (
	netName = "172.22.16.255"
)

var (
	beatMsg = buildBeatMsg()
	enc     = binary.LittleEndian
)

func buildBeatMsg() []byte {
	ret := make([]byte, 24)
	enc.PutUint64(ret[0:8], 1e9)
	enc.PutUint64(ret[8:16], 2e9)
	enc.PutUint64(ret[16:24], 3e9)

	return ret
}

func NewBeater() *Beater {
	ret := new(Beater)
	var e error

	ret.addr1, e = net.ResolveUDPAddr("udp4", netName+":7340")
	noError(e)

	ret.addr2, e = net.ResolveUDPAddr("udp4", netName+":7339")
	noError(e)

	ret.conn, e = net.ListenUDP("udp4", nil)
	noError(e)

	return ret
}

func (self *Beater) Beat() {
	_, e := self.conn.WriteTo(beatMsg, self.addr1)
	noError(e)

	_, e = self.conn.WriteTo(beatMsg, self.addr2)
	noError(e)
}

func Serve(addr string) {
	a, e := net.ResolveTCPAddr("tcp4", addr)
	noError(e)

	server, e := net.ListenTCP("tcp4", a)
	noError(e)

	beater := NewBeater()

	for {
		c, e := server.Accept()
		noError(e)

		reader := bufio.NewReader(c)
		_, e = reader.ReadBytes('\n')
		if e != nil {
			log.Println(e)
			continue
		}

		log.Println("beat:", c.RemoteAddr().String())
		beater.Beat()

		c.Close()
	}
}
