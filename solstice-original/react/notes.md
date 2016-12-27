# Todo

- try bfs search
- try normalized demand (approaching 100%)
- a better trim, align and adjust
- closed loop simulation (in the simulator)
- think about a distributed scheduler
- think about hardware implementation

# Delayed todos

- add ethernet packet into the simulator
- pdump package, for saving packet traces: sim.send sim.recv
- replay can (and should) be converted from packet dumps
- dumping on the switch, rx and ctrl
- latency tracking
- counter statistics gathering
- workload generate and analysis (will need 2 traffic classes)
- better replay plotting

# Trimming and aligning

- It does not matter how much is left on packet. It is a packet switch, so just leave it there
- What is more important is that there should not be so many days scheduled on circuit
- And when it is scheduled

