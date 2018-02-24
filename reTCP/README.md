# reTCP (Reconfigurable Datacenter Network TCP)

reTCP is a pluggable linux congestion control module that looks for changes in
the ECE (ECN-Echo) bit in TCP ACKs. If the ECE bit transitions from 0 to 1,
reTCP will multiply ```cwnd``` by a user-settable factor ```jump_up``` upon the
next ```cwnd``` update (i.e., when ```.cong_avoid``` is called). When the ECE
bit transitions from 1 to 0, ```cwnd``` is divided by a user-settable factor
```jump_down``` upon the next ```cwnd``` update. By default both parameters are
2. This seems to work well with our reconfigurable switch emulator.

Additionally, a kernel change is needed to make sure changes in the ECE bit are
propagated to the module without forcing ECN to be enabled (as this would cause
the ```cwnd``` to decrease upon receiving packets with ECE set).
