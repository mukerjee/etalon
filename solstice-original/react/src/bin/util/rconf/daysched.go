package main

import (
	"time"
)

/*
To generate a day schedule, we have several steps

The orig
0. a mapping from source host to dest host for each day
1. a mapping from host id to host id for each day
2. queue id for each day for each end host
3. a mapping from input port to output port for each day for the switch

we can skip 0. for now
*/

type Day struct {
	dayTime      time.Duration
	dayUnpause   Match
	nightUnpause Match
}
