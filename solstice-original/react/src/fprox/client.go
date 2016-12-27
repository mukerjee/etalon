package fprox

import (
	"bytes"
	"io"
	"io/ioutil"
	"log"
	"net"
	"time"
)

type Client struct {
	Verbose bool
	Name    string
	addr    string
}

func NewClient(addr string) (*Client, error) {
	ret := new(Client)
	ret.addr = addr

	return ret, nil
}

func (self *Client) Read(path string) (string, error) {
	reader, e := self.Open(path)
	if e != nil {
		return "", e
	}

	buf, e := ioutil.ReadAll(reader)
	if e != nil {
		return "", e
	}
	if e = reader.Close(); e != nil {
		return "", e
	}

	return string(buf), nil
}

func (self *Client) Write(path string, s string) error {
	conn, e := self.Create(path)
	if e != nil {
		return e
	}

	_, e = conn.Write([]byte(s))
	if e != nil {
		return e
	}
	e = conn.CloseWrite()
	if e != nil {
		return e
	}

	buf := []byte{0}
	_, e = conn.Read(buf)
	if e != nil {
		return e
	}

	conn.Close()
	return nil
}

func (self *Client) dial() (*net.TCPConn, error) {
	conn, e := net.DialTimeout("tcp4", self.addr, time.Second)
	if e != nil {
		return nil, e
	}

	return conn.(*net.TCPConn), nil
}

func (self *Client) Open(path string) (io.ReadCloser, error) {
	if self.Verbose {
		log.Println(self.Name, "read", path)
	}

	conn, e := self.dial()
	if e != nil {
		return nil, e
	}

	buf := new(bytes.Buffer)
	buf.WriteString("read\n")
	buf.WriteString(path + "\n")
	_, e = conn.Write(buf.Bytes())
	if e != nil {
		return nil, e
	}

	return conn, nil
}

func (self *Client) Create(path string) (*net.TCPConn, error) {
	if self.Verbose {
		log.Println(self.Name, "write", path)
	}

	conn, e := self.dial()
	if e != nil {
		return nil, e
	}

	buf := new(bytes.Buffer)
	buf.WriteString("write\n")
	buf.WriteString(path + "\n")
	_, e = conn.Write(buf.Bytes())
	if e != nil {
		return nil, e
	}

	return conn, nil
}
