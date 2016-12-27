package main

type Match []int

const NoMatch = -1

func isValidHost(i int) bool {
	return 0 <= i && i < nhost
}

func MapMatch(m map[int]int) Match {
	ret := EmptyMatch()

	for src, dest := range m {
		assert(isValidHost(src))
		assert(isValidHost(dest))
		assert(ret[dest] == NoMatch)

		ret[dest] = src
	}

	return ret
}

func EmptyMatch() Match {
	ret := make([]int, nhost)
	for i := range ret {
		ret[i] = NoMatch
	}

	return ret
}
