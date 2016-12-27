package ctrlh

import (
	"encoding/binary"
	"encoding/hex"
	"fmt"
	"io"
)

func ctrlOp(w io.Writer, op uint16, n uint16) error {
	buf := make([]byte, 4)
	binary.LittleEndian.PutUint16(buf[0:2], op)
	binary.LittleEndian.PutUint16(buf[2:4], n)

	_, e := w.Write(buf)
	return e
}

var writeVerbose = false

func writePacket(w io.Writer, op uint16, p *Packet) error {
	if p == nil {
		panic("nil packet")
	}

	buf := p.Pack()
	if writeVerbose {
		fmt.Printf("(send %d bytes)\n", len(buf))
		fmt.Print(hex.Dump(buf))
	}

	n := len(buf)
	if n > MaxPacketLen {
		panic("packet too large")
	}

	e := ctrlOp(w, op, uint16(n))
	if e != nil {
		return e
	}

	_, e = w.Write(buf)
	return e
}

const (
	OpSleepMs   = iota // sleep for several milliseconds
	OpSend             // send a packet immediately
	OpSendOnSig        // save a packet in buffer
	OpWaitSig          // send all the saved packets on receiving a weeksig
)

func CtrlSleepMs(w io.Writer, ms uint16) error {
	return ctrlOp(w, OpSleepMs, ms)
}

func CtrlSleep(w io.Writer, sec uint16) error {
	if sec > 60 {
		panic("sleeping too long")
	}

	return ctrlOp(w, OpSleepMs, sec*1000)
}

func CtrlSend(w io.Writer, p *Packet) error {
	return writePacket(w, OpSend, p)
}

func CtrlSendOnSig(w io.Writer, p *Packet) error {
	return writePacket(w, OpSendOnSig, p)
}

func CtrlWaitSig(w io.Writer, meta uint16) error {
	return ctrlOp(w, OpWaitSig, meta)
}
