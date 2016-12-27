package rawpack

import (
	"encoding/binary"
)

var enc = binary.BigEndian
var BroadcastMAC = MAC("ffff.ffff.ffff")

const (
	ipLen  = 20
	udpLen = 8

	ipEthType = 0x0800
	udpProto  = 0x11

	defaultIPId = 7337
	defaultTTL  = 64

	minPackSize = 60
)
