package queues

type Allocator struct {
	capacity uint64
	used     uint64
}

func NewAllocator(capacity uint64) *Allocator {
	ret := new(Allocator)
	ret.capacity = capacity
	return ret
}

func (self *Allocator) Request(a uint64) uint64 {
	if self.capacity == self.used {
		return 0
	}

	assert(self.capacity >= self.used)
	left := self.capacity - self.used
	if left < a {
		return left
	}
	return a
}

func (self *Allocator) Alloc(a uint64) uint64 {
	if self.capacity == self.used {
		return 0
	}

	assert(self.capacity >= self.used)
	left := self.capacity - self.used
	if left < a {
		self.used = self.capacity
		return left
	}

	self.used += a
	return a
}

func (self *Allocator) Free(a uint64) {
	assert(self.used >= a)
	self.used -= a
}
