package pqueues

import (
	"container/heap"
)

var _ heap.Interface = new(flowHeap)

// A heap of flowQueues
type flowHeap struct {
	flows   []*flowQueue   // this the heap
	flowMap map[uint64]int // an index, maps flow id to position in flows
}

func newFlowHeap() *flowHeap {
	ret := new(flowHeap)
	ret.flowMap = make(map[uint64]int)
	return ret
}

func (self *flowHeap) Len() int {
	return len(self.flows)
}

func (self *flowHeap) Less(i, j int) bool {
	flowi := self.flows[i]
	flowj := self.flows[j]

	if flowi.lastTouched < flowj.lastTouched {
		return true
	}
	if flowi.lastTouched > flowj.lastTouched {
		return false
	}

	leni := flowi.Size()
	lenj := flowj.Size()

	if leni < lenj {
		return true
	}
	if leni > lenj {
		return false
	}
	return flowi.Flow < flowj.Flow
}

func (self *flowHeap) Swap(i, j int) {
	flowi := self.flows[i]
	flowj := self.flows[j]

	assert(self.flowMap[flowi.Flow] == i)
	assert(self.flowMap[flowj.Flow] == j)

	self.flowMap[flowi.Flow] = j // will map to j now
	self.flowMap[flowj.Flow] = i // will map to i now

	self.flows[i], self.flows[j] = flowj, flowi
}

func (self *flowHeap) HasFlow(f uint64) bool {
	_, found := self.flowMap[f]
	return found
}

func (self *flowHeap) Push(x interface{}) {
	q := x.(*flowQueue)

	assert(!self.HasFlow(q.Flow))

	n := len(self.flows)
	self.flows = append(self.flows, q)
	self.flowMap[q.Flow] = n
}

func (self *flowHeap) Pop() interface{} {
	last := len(self.flows) - 1
	ret := self.flows[last]
	self.flows = self.flows[0:last]

	assert(self.flowMap[ret.Flow] == last)
	delete(self.flowMap, ret.Flow)

	return ret
}
