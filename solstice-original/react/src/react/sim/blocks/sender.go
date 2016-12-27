package blocks

import (
	. "react/sim/packet"
)

// interface with back pressure
// this is the input interface for a switch
type Sender interface {
	Send(packet *Packet) uint64
}
