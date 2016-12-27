package tors

import (
	. "react/sim/config"
	. "react/sim/queues"
)

func NewNics() *Queues {
	return NewSizedQueues(NicBufSize())
	// return NewNicQueues(NicBufSize(), NicBufTotalSize())
}
