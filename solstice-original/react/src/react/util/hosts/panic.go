package hosts

import (
	"errors"
)

var errAssert = errors.New("assertion failed")

func assert(cond bool) {
	if !cond {
		panic(errAssert)
	}
}

func noErr(e error) {
	assert(e == nil)
}
