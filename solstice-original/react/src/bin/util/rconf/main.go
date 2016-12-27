package main

func main() {
	control := testControl

	config := NewConfig()
	config.TimeForMindspd()
	lanes := "f0 d0 d3 f3  f0 d1 d2 f2"
	config.TxWithLanes(lanes)
	config.RxWithLanes(lanes)
	config.SetSlaveLocation("sw")

	config.SendPFC(true)
	config.SendWeekend(true)
	config.SendWeekSignal(true)
	config.SyncInternally()

	config.Watch("f3", "rx")

	control.Setup(config)
}
