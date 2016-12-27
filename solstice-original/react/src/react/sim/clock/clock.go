package clock

var T uint64

func Reset() {
	T = 0
}

func Tick() {
	T++
}
