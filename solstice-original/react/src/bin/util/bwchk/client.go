package main

import (
	"fmt"
	"net"
	"time"
)

// a client that supports long-lasting big flows and
// and also repeative small flows
type Client struct {
	addr     *net.TCPAddr
	buf      []byte
	total    int
	interval time.Duration
	conn     *net.TCPConn
	sent     int

	switchState   bool
	running       bool
	stopRequested bool
	stopSignal    chan bool
	startSignal   chan bool
}

func NewClient(host string) *Client {
	return NewRepeatClient(host, 0, 0)
}

func NewRepeatClient(host string, each int, interval time.Duration) *Client {
	ret := new(Client)

	dest := fmt.Sprintf("%s:%d", host, port)
	var err error
	ret.addr, err = net.ResolveTCPAddr("tcp4", dest)
	noErr(err)
	ret.buf = make([]byte, parseBufSize(bufSize))

	assert(each >= 0)
	ret.total = each
	assert(interval >= 0)
	assert(interval <= time.Second)
	ret.interval = interval

	ret.running = true
	ret.switchState = true
	ret.stopSignal = make(chan bool, 1)
	ret.startSignal = make(chan bool, 1)

	return ret
}

func (self *Client) stopped() bool {
	if self.stopRequested {
		return true
	}
	if len(self.stopSignal) > 0 {
		<-self.stopSignal
		self.stopRequested = true
		return true
	}
	return false
}

func (self *Client) sleep(duration time.Duration) bool {
	after := time.After(duration)
	assert(!self.stopRequested)

	select {
	case <-after:
		return true
	case <-self.stopSignal:
		self.stopRequested = true
		return false // interrupted
	}
}

func (self *Client) wait(duration time.Duration) bool {
	after := time.After(duration)
	select {
	case <-after:
		return false
	case <-self.startSignal:
		return true
	}
}

func (self *Client) Stop() {
	if self.switchState {
		self.switchState = false
		self.stopSignal <- true
	}
}

func (self *Client) Start() {
	if !self.switchState {
		self.switchState = true
		self.startSignal <- true
	}
}

func (self *Client) dial() {
	assert(self.conn == nil)

	var err error
	self.conn, err = net.DialTCP("tcp4", nil, self.addr)
	if err != nil {
		self.sleep(time.Second / 4)
		self.conn = nil
		return
	}

	err = self.conn.SetLinger(0) // discard unsent data when closing
	noErr(err)
	self.sent = 0
}

func (self *Client) send() {
	assert(self.conn != nil)

	buf := self.buf
	if self.total > 0 {
		// finite flow
		left := self.total - self.sent
		assert(left > 0)
		if left < len(buf) {
			buf = buf[:left]
		}
	}

	// it appears that if a deadline is not set, golang will
	// try to send out the entire buffer in one call
	// but for our application, it needs to be more reactive
	deadline := time.Now().Add(time.Millisecond / 20)
	err := self.conn.SetWriteDeadline(deadline)
	noErr(err)

	n, err := self.conn.Write(buf)
	if err != nil {
		opError := err.(*net.OpError)
		if !opError.Temporary() {
			// fmt.Println(err)
			// write error, connection lost
			self.conn.Close()
			self.conn = nil
			self.sleep(time.Second / 4)
			return
		}
	}

	// fmt.Println(n)
	self.sent += n

	if self.sent == self.total {
		self.conn.Close()
		self.conn = nil

		if self.interval > 0 {
			self.sleep(self.interval)
			return
		}
	}
}

func (self *Client) Run() {
	for running() {
		if !self.running {
			if !self.wait(time.Second / 2) {
				continue
			}

			self.running = true // start request handled
		} else {
			if self.stopped() {
				if self.conn != nil {
					self.conn.Close()
					self.conn = nil
				}

				self.running = false
				self.stopRequested = false // stop request handled
			} else {
				if self.conn == nil {
					self.dial()
				} else {
					self.send()
				}
			}
		}
	}
}
