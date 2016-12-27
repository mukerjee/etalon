package react

import "fmt"

type ViewClient struct {
	_ int
}

var _ View = new(ViewClient)

func NewViewClient() *ViewClient {
	ret := new(ViewClient)
	return ret
}

func (self *ViewClient) send(cmd string, a ...interface{}) {
	s := fmt.Sprint(cmd, " ") + fmt.Sprint(a...)
	logln(s)
}

func (self *ViewClient) Reset(nhost int) {
	self.send("Reset", nhost)
}

func (self *ViewClient) ConfigTime(weekLen, dayLen, nightLen uint64) {
	self.send("ConfigTime", weekLen, dayLen, nightLen)
}

func (self *ViewClient) ConfigBandw(linkBw, packBw uint64) {
	self.send("ConfigBandw", linkBw, packBw)
}

func (self *ViewClient) Time(t uint64) {
	self.send("Time", t)
}

func (self *ViewClient) MatrixClear(id int) {
	self.send("MatrixClear", id)
}

func (self *ViewClient) MatrixTitle(id int, s string) {
	self.send("MatrixTitle", id, s)
}

func (self *ViewClient) MatrixNorm(id int, one uint64) {
	self.send("MatrixNorm", id, one)
}

func (self *ViewClient) Matrix(id int, m Matrix) {
	if m == nil {
		return
	}
	self.send("Matrix", id, m.Marshal())
}

func (self *ViewClient) SchedClearAfter(i int) {
	self.send("SchedClear", i)
}

func (self *ViewClient) Sched(i int, day *Day) {
	slice := day.s
	self.send("Sched", i, slice.id, slice.weight, slice.data.Marshal())
}

func (self *ViewClient) SchedTime(day int, timeOfDay uint64, isNight bool) {
	self.send("SchedTime", day, timeOfDay, isNight)
}

func (self *ViewClient) FlowClearAfter(i int) {
	self.send("FlowClear", i)
}

func (self *ViewClient) Flow(i int, flow *Flow) {
	self.send("Flow", i, flow.lane, flow.size, flow.sent,
		flow.acked, flow.rate, flow.stable)
}

func (self *ViewClient) Message(s string) {
	self.send("//", s)
}

func (self *ViewClient) Close() {
	self.send("Close")
}
