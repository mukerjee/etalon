import os

class DilationAdaptor(object):
	"""super class for these who can dynamically return a new tdf"""
	def __init__( self, old_tdf, load = None):
		super(DilationAdaptor, self).__init__()
		self.load = load
		self.current_tdf = old_tdf

	def decideNewDilatonFactor( self ):
		"override this function to provide custom adaptive algorithm"
		return self.current_tdf

	def updateLoad( self, load ):
		self.load = load

	def updateCurrentTDF( self, tdf):
		self.current_tdf = tdf

class TailoredForStringTopo(DilationAdaptor):
	"""Double the TDF when cpu load beyond threshold"""
	def __init__(self, old_tdf, threshold, load = None):
		super(TailoredForStringTopo, self).__init__(old_tdf, load)
		self.threshold = threshold

	def decideNewDilatonFactor( self ):
		assert self.load
		old_tdf = self.current_tdf
		new_tdf = old_tdf
		if self.load > self.threshold:
			new_tdf = old_tdf * 2
		if new_tdf > 80:
			new_tdf = 80
		self.current_tdf = new_tdf
		return new_tdf

class LoadState:
	PANIC, WARNING, RESONABLE, UNDERLOAD = range(4)

class ThresholdBasedAdaptor(DilationAdaptor):
	"""implementation from Grau's 2009 paper"""
	def __init__(self, old_tdf, resonable, warning, panic, load = None):
		super(ThresholdBasedAdaptor, self).__init__(old_tdf, load)
		self.resonable = resonable
		self.warning = warning
		self.panic = panic
		self.small_change = 1
		self.large_change = 10
		self.upper_bound = 80

	def decideLoadState( self ):
		load = self.load
		if load >= self.panic:
			return LoadState.PANIC
		elif load >= self.warning:
			return LoadState.WARNING
		elif load >= self.resonable:
			return LoadState.RESONABLE
		else:
			return LoadState.UNDERLOAD

	def decideNewDilatonFactor( self ):
		assert self.load
		state = self.decideLoadState()
		old_tdf = self.current_tdf
		if state == LoadState.PANIC:
			new_tdf = old_tdf + self.large_change

		elif state == LoadState.WARNING:
			new_tdf = old_tdf + self.small_change * 2

		elif state == LoadState.RESONABLE:
			if old_tdf == self.upper_bound:
				new_tdf = old_tdf - self.large_change
			elif old_tdf > self.small_change:
				new_tdf = old_tdf - self.small_change
			else:
				new_tdf = old_tdf
		elif state == LoadState.UNDERLOAD:
			if old_tdf > self.large_change * 2:
				new_tdf = old_tdf - self.large_change
			elif old_tdf >= self.small_change * 4:
				new_tdf = old_tdf - self.small_change * 2
			elif old_tdf >= self.small_change * 2:
				new_tdf = old_tdf - self.small_change
			else:
				new_tdf = old_tdf
		else:
			new_tdf = old_tdf

		if new_tdf > self.upper_bound:
			new_tdf = self.upper_bound
		if new_tdf < 0:
			print "Error: change TDF to negtive value; reset it to 1"
			new_tdf = 1

		self.current_tdf = new_tdf
		return new_tdf

def testAdaptors():
	tba = ThresholdBasedAdaptor(1, 5, 20, 40, 32)
	print tba.decideNewDilatonFactor()

	cpu_usage = [25, 75, 2, 30, 35, 15, 60, 70, 4]
	for cu in cpu_usage:
		tba.updateLoad(cu)
		print tba.decideNewDilatonFactor()

	tfst = TailoredForStringTopo(1, 28)
	cpu_usage = [25, 75, 2, 30, 35, 15, 60, 70, 4]
	for cu in cpu_usage:
		tfst.updateLoad(cu)
		print tfst.decideNewDilatonFactor()


if __name__ == '__main__':
	testAdaptors()




