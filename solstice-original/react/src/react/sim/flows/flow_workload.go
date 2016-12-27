package flows

type FlowWorkload interface {
	Tick(driver *FlowDriver)
}
