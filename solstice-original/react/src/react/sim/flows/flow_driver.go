package flows

import (
	"encoding/json"
	"io"

	"react/sim/clock"
	. "react/sim/config"
	// "react/sim/stats"
	"react/sim/packet"
	"react/sim/structs"
)

type query struct {
	tstart    uint64
	responded int
	respSizes structs.Vector
}

func newQuery() *query {
	ret := new(query)
	ret.tstart = clock.T
	// ret.responded = 0
	return ret
}

// A FlowDriver is responsible for creating flows
// It resembles application logic, like simple
// bulk data moving and partition-aggregation pattern
type FlowDriver struct {
	Workload FlowWorkload

	Keeper *FlowKeeper

	queries  map[uint64]*query
	querySeq uint64

	// stats
	/*
		tinyFlowSizes  *stats.BinCounter
		smallFlowSizes *stats.BinCounter
		bigFlowSizes   *stats.BinCounter

		tinyFlowTimeCosts  *stats.BinCounter
		smallFlowTimeCosts *stats.BinCounter
		bigFlowTimeCosts   *stats.BinCounter
		queryFlowTimeCosts *stats.BinCounter
		queryTimeCosts     *stats.BinCounter

		tinyFlowCircuit, tinyFlowPacket   uint64
		smallFlowCircuit, smallFlowPacket uint64
		bigFlowCircuit, bigFlowPacket     uint64
		queryFlowCircuit, queryFlowPacket uint64
	*/
	LogTo io.Writer
}

type FlowRecord struct {
	Id      uint64
	From    int
	To      int
	Type    string
	Size    uint64
	Start   uint64
	End     uint64
	ViaPack uint64
	Class   string
}

func NewFlowDriver() *FlowDriver {
	ret := new(FlowDriver)

	ret.queries = make(map[uint64]*query)

	/*
		ret.tinyFlowSizes = stats.NewBinCounter(100000)
		ret.smallFlowSizes = stats.NewBinCounter(100000)
		ret.bigFlowSizes = stats.NewBinCounter(100000)

		ret.tinyFlowTimeCosts = stats.NewBinCounter(100000)
		ret.smallFlowTimeCosts = stats.NewBinCounter(100000)
		ret.bigFlowTimeCosts = stats.NewBinCounter(100000)
		ret.queryFlowTimeCosts = stats.NewBinCounter(100000)
		ret.queryTimeCosts = stats.NewBinCounter(100000)
	*/

	return ret
}

func (self *FlowDriver) logFlowEnd(flow *Flow, t string) {
	if self.LogTo == nil {
		return
	}
	var entry FlowRecord
	entry.Id = flow.id
	entry.From, entry.To = structs.RowCol(flow.lane)
	entry.Size = flow.size
	entry.Start = flow.tstart
	entry.End = clock.T
	entry.Type = t
	entry.ViaPack = flow.viaPacket
	if flow.hint == packet.Elephant {
		entry.Class = "elephant"
	} else if flow.hint == packet.Mouse {
		entry.Class = "mouse"
	} else {
		entry.Class = "na"
	}

	self.writeLog(&entry)
}

func (self *FlowDriver) writeLog(entry *FlowRecord) {
	bytes, e := json.Marshal(&entry)
	if e != nil {
		panic("bug")
	}

	_, e = self.LogTo.Write(bytes)
	if e != nil {
		panic(e)
	}

	_, e = self.LogTo.Write([]byte("\n"))
	if e != nil {
		panic(e)
	}
}

func (self *FlowDriver) logQueryEnd(server int, q *query) {
	if self.LogTo == nil {
		return
	}
	var entry FlowRecord
	entry.From = server
	entry.To = 0
	entry.Size = q.respSizes.Sum()
	entry.Start = q.tstart
	entry.End = clock.T
	entry.Type = "query"

	self.writeLog(&entry)
}

func (self *FlowDriver) Close(flow *Flow) {
	// logln(flow, "//ends")
	switch flow.class {
	case QueryRequest:
		// send a query respond flow
		lane := flow.lane
		src := lane / Nhost
		dest := lane % Nhost
		dualLane := dest*Nhost + src
		id := flow.Payload
		query := self.queries[id]

		respondFlow := NewFlow(dualLane,
			query.respSizes[dest], QueryRespond, self)
		respondFlow.Payload = id
		self.Keeper.add(respondFlow)

		// TODO: keep stats at other places
		// self.queryFlowCircuit += flow.viaCircuit
		// self.queryFlowPacket += flow.viaPacket
		// logln(respondFlow)
	case QueryRespond:
		id := flow.Payload
		query := self.queries[id]
		query.responded++

		/*
			timeCost := int(clock.T - query.tstart)
			self.queryFlowTimeCosts.Count(timeCost)
			self.queryFlowCircuit += flow.viaCircuit
			self.queryFlowPacket += flow.viaPacket
		*/

		if query.responded == Nhost-1 {
			_, server := structs.RowCol(flow.lane)

			// TODO: keep stats at other places
			// self.queryTimeCosts.Count(timeCost)
			self.logQueryEnd(server, query)

			delete(self.queries, id)
		}

	case Background:
		self.logFlowEnd(flow, "bg")
		// TODO: keep stats at other places
		/*
			timeCost := int(clock.T - flow.tstart)
			viaCircuit := flow.viaCircuit
			viaPacket := flow.viaPacket
			if flow.size < uint64(1e4) {
				self.tinyFlowTimeCosts.Count(timeCost)
				self.tinyFlowCircuit += viaCircuit
				self.tinyFlowPacket += viaPacket
			} else if flow.size < uint64(1e6) {
				self.smallFlowTimeCosts.Count(timeCost)
				self.smallFlowCircuit += viaCircuit
				self.smallFlowPacket += viaPacket
			} else {
				self.bigFlowTimeCosts.Count(timeCost)
				self.bigFlowCircuit += viaCircuit
				self.bigFlowPacket += viaPacket
			}
		*/
	}
}

/*
func (self *FlowDriver) recordFlow(flow *Flow) {
	size := int(flow.size)
	if size < int(1e4) {
		self.tinyFlowSizes.Count(size)
	} else if size < int(1e6) {
		self.smallFlowSizes.Count(size)
	} else {
		self.bigFlowSizes.Count(size)
	}
}
*/

func (self *FlowDriver) AddBackgroundFlow(lane int, size uint64) {
	flow := NewFlow(lane, size, Background, self)
	self.Keeper.add(flow)
	// self.recordFlow(flow)
	// logln(flow)
}

func (self *FlowDriver) AddMouseFlow(lane int, size uint64) {
	flow := NewFlow(lane, size, Background, self)
	flow.hint = packet.Mouse
	self.Keeper.add(flow)
}

func (self *FlowDriver) AddElephantFlow(lane int, size uint64) {
	flow := NewFlow(lane, size, Background, self)
	flow.hint = packet.Elephant
	self.Keeper.add(flow)
}

func (self *FlowDriver) AddQueryFlow(src int, reqSize uint64,
	respSizes structs.Vector) {
	laneBase := src * Nhost
	seq := self.querySeq
	for dest := 0; dest < Nhost; dest++ {
		if dest == src {
			continue
		}
		lane := laneBase + dest
		flow := NewFlow(lane, reqSize, QueryRequest, self)
		flow.Payload = seq
		self.Keeper.add(flow)
		// logln(flow)
	}
	query := newQuery()
	query.respSizes = respSizes
	self.queries[seq] = query
	self.querySeq++
}

func (self *FlowDriver) tick() {
	if self.Workload != nil {
		self.Workload.Tick(self)
	}
}
