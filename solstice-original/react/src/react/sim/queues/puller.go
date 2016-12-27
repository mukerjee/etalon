package queues

import (
	. "react/sim/structs"
)

type Puller interface {
	Pull(budget Matrix) Matrix
}
