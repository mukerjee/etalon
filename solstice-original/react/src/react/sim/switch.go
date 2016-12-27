package sim

import (
	"react/sim/blocks"
	. "react/sim/structs"
)

type Switch interface {
	// Tells if the switch is a TDMA switch.
	Tdma() bool

	/*
		A switch is a sender where other parts can
		queue some packet onto the switch.
		Nics queues are also part of the switch.
	*/
	blocks.Sender

	/*
		Moves packet inside the switch to the sink.

		If the switch is a TDMA switch (a circuit switch or a
		hybrid switches), it uses the estimator to estimate
		the demand in the near future. Returns the schedule for
		current tick, and the schedule events that happens at
		the end of the tick.

		If the switch is not a TDMA switch (a packet switch),
		then it just moves packets, and always returns (nil, 0)
	*/
	Tick(sink blocks.Block, estimator Estimator) (Matrix, Events)

	// Sets the switch to use a particular scheduler
	Bind(s Scheduler, logger *SchedRecorder)

	Served() (circ Matrix, pack Matrix)
}
