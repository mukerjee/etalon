package ctrlh

import (
	"react/testb/setup"
)

const (
	routeCtrl   = 0xfc
	routeReset  = 0xe1
	routeSwitch = 0xe2
)

func routeEndHost(id int) byte {
	if !setup.CheckId(id) {
		panic("invalid id")
	}

	return byte(0xf0 + id)
}
