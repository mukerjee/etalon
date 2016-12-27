package bvn

import (
	. "react/sim/config"
	. "react/sim/structs"
)

type IslipSlicer struct {
	acceptPt   PortMap
	acceptStep PortMap
	grantPt    PortMap
	grant      PortMap
	reverse    PortMap

	Iteration int
}

var _ Slicer = new(IslipSlicer)

func NewIslipSlicer() *IslipSlicer {
	ret := new(IslipSlicer)

	ret.acceptPt = DirectPortMap()
	ret.acceptStep = NewPortMap()

	ret.grantPt = DirectPortMap()
	ret.grant = NewPortMap()

	ret.reverse = NewPortMap()
	ret.Iteration = 1

	return ret
}

func (self *IslipSlicer) Reset() {}

func (self *IslipSlicer) Slice(m Sparse, pm PortMap) int {
	pm.Clear()
	self.reverse.Clear()
	ret := 0

	assert(self.Iteration > 0)

	for iter := 0; iter < self.Iteration; iter++ {
		// println("iter:", iter)
		self.grant.Clear()

		// checking requests and handing out grants
		for dest, scol := range m {
			if self.reverse[dest] >= 0 {
				// already matched
				continue
			} else {
				assert(self.reverse[dest] == InvalidPort)
			}
			if len(scol) == 0 {
				// no request received
				continue // XXX: might need to double-check on this
			}
			cur := self.grantPt[dest]

			cur = (cur + 1) % Nhost
			minStep := Nhost
			for src, a := range scol {
				if pm[src] >= 0 {
					continue // already matched, don't request
				} else {
					assert(pm[src] == InvalidPort)
				}
				assert(a > 0)
				step := (src + Nhost - cur) % Nhost
				if step < minStep {
					minStep = step
				}
			}

			if minStep >= Nhost {
				// nobody requested
				assert(minStep == Nhost)
				self.grant[dest] = InvalidPort
				// grantPt will remain unchanged
			} else {
				self.grantPt[dest] = (cur + minStep) % Nhost
				self.grant[dest] = (cur + minStep) % Nhost
			}
		}

		// grants recieved
		// picking one to accept
		self.acceptStep.Clear()
		for dest, srcGrant := range self.grant {
			if srcGrant < 0 {
				// granting nobody
				continue
			}

			curDest := self.acceptPt[srcGrant]
			step := (dest + Nhost - curDest - 1) % Nhost
			curStep := self.acceptStep[srcGrant]
			if curStep < 0 || step < curStep {
				self.acceptStep[srcGrant] = step // accepting
			}
		}

		// final accept
		for src, destStep := range self.acceptStep {
			if destStep < 0 {
				continue
			}

			curDest := self.acceptPt[src]
			newDest := (curDest + 1 + destStep) % Nhost
			// println(src, newDest)
			assert(pm[src] == InvalidPort)
			assert(self.reverse[newDest] == InvalidPort)
			pm[src] = newDest            // accepting
			self.acceptPt[src] = newDest // update pointer
			self.reverse[newDest] = src
			ret++
		}
	}

	// println("--")
	return ret
}
