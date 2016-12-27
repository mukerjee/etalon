package react

type View interface {
	Reset(nhost int)

	ConfigTime(weekLen, dayLen, nightLen uint64)
	ConfigBandw(linkBw, packBw uint64)

	Time(t uint64)

	MatrixClear(id int)
	MatrixTitle(id int, title string)
	MatrixNorm(id int, one uint64)
	Matrix(id int, m Matrix)

	SchedClearAfter(i int)
	Sched(i int, day *Day)
	SchedTime(day int, timeOfDay uint64, isNight bool)

	FlowClearAfter(i int)
	Flow(i int, flow *Flow)

	Message(s string)

	Close()
}
