package main

func assert(cond bool) {
	if !cond {
		panic("bug")
	}
}
