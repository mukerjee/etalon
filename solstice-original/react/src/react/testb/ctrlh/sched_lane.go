package ctrlh

type SchedLane struct {
	Src  int
	Dest int
	Rate int
}

func (self *SchedLane) Qid() int {
	return relDest(self.Src, self.Dest)
}
