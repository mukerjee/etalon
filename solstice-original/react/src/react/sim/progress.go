package sim

type Progress interface {
	Start(s *ProgressStat)
	Tick(s *ProgressStat)
	Stop(s *ProgressStat)
}
