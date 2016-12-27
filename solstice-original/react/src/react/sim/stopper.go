package sim

type Stopper interface {
	ShouldStop() bool
}
