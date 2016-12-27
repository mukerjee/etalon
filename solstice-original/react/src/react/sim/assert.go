package sim

import "errors"

var errAssert = errors.New("Assertion Failed.")

func assert(cond bool) {
	if !cond {
		panic(errAssert)
	}
}

func noError(e error) {
	if e != nil {
		panic(e)
	}
}
