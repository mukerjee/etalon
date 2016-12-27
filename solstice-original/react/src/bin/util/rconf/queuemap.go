package main

type QueueMap map[int]int

func makeRingQueueMaps(n int) []QueueMap {
	assert(n <= 8)

	ret := make([]QueueMap, n)
	for src := range ret {
		m := make(QueueMap)

		for dest := 1; dest < n; dest++ {
			m[dest] = (dest + n - src) % n
		}

		ret[src] = m
	}
	return ret
}

func makeConstQueueMaps(n int) []QueueMap {
	assert(n <= 7)

	ret := make([]QueueMap, n)
	for src := range ret {
		m := make(QueueMap)

		for dest := 0; dest < n; dest++ {
			m[dest] = dest + 1 // 0 is reserved for packet queue
		}

		ret[src] = m
	}
	return ret
}
