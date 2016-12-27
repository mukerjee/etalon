package sim

const (
	Noop    Events = 0
	NewWeek Events = 0x1
	Dusk    Events = 0x2
	Dawn    Events = 0x4
)

type Events uint64

func (self Events) Test(e Events) bool  { return (self & e) == e }
func (self Events) Set(e Events) Events { return (self | e) }
