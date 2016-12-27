package flows

import "errors"

var errAssert = errors.New("Assertion Failed.")

func assert(cond bool) {
	if !cond {
		panic(errAssert)
	}
}
