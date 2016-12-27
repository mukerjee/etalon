package sim

type ProgressStat struct {
	T        uint64 // time now
	Goodput  uint64 // Bytes delivered
	Circput  uint64 // Bytes delivered via circuit
	Packput  uint64 // Bytes delivered via packet
	Dropped  uint64 // Bytes dropped
	Capacity uint64 // Bytes could deliver
}
