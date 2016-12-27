package ctrlh

const (
	cmdUnknown = 0

	cmdSchedSet   = 0x80
	cmdSchedSwap  = 0x81
	cmdReset      = 0x90
	cmdAddr       = 0x91
	cmdFlags      = 0x92
	cmdWeeksig    = 0x93
	cmdTimings    = 0x94
	cmdLaneSelect = 0x95
	cmdStat       = 0x96
	cmdLoopback   = 0x97
	cmdSender     = 0x9f

	cmdDebugSelect = 0xdb
)
