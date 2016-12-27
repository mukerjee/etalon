package sim

import (
	"react/sim/blocks"
)

type Hosts interface {
	// Called every tick, may send packets to the sender,
	// which will be relayed to the switch.
	Tick(sender blocks.Sender)

	// Hosts are blocks. They receive packets from the switch.
	blocks.Block
}
