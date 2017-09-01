import matplotlib.pyplot as pyplot
import matplotlib
import numpy
def drawData(output):
	base_category = (1, 2, 3)
	dataLables = ['Mininet, TDF=1', 'Mininet, TDF=4', 'Physical Testbed']
	xLabel = 'Link Bandwidth (Gbps)'
	yLabel = 'Average TCP Throughput (Gbps)'

	color_list = ['c', 'r', 'm', 'y', 'g', 'b', 'k', 'w']
	hatch_list = ['/', '\\', '+', 'x', 'o', '.', '*', '-']
	width = 0.2
	fontSize = 40
	maxY = 10

	rects = []
	fig, ax = pyplot.subplots()
	for index in [0, 1, 2]:
		category = [x + index * width for x in base_category]
		rect = ax.bar(category, AvgRate[index], width, color=color_list[index], yerr=StdRate[index], hatch=hatch_list[index])
		rects.append(rect)

	ax.legend(tuple(rects), dataLables, shadow=True, fancybox=True, fontsize=fontSize, loc='upper left')
	ax.set_xticks([x + width*3/2 for x in base_category ])
	ax.set_xticklabels(('4', '8', '10'))
	ax.set_yticks(range(maxY+1))


	for tick in ax.xaxis.get_major_ticks():
		tick.label.set_fontsize(fontSize)
	for tick in ax.yaxis.get_major_ticks():
		tick.label.set_fontsize(fontSize)

	pyplot.grid()
	pyplot.ylim(( 0, 10 ))
	pyplot.xlim(0.65, len(base_category) + 1)
	pyplot.xlabel(xLabel, fontsize=fontSize)
	pyplot.ylabel(yLabel, fontsize=fontSize)
	# pyplot.yticks([x for x in range(0, 1, 11)])
	# pyplot.xticks([x for x in range(1, 4, 1)])
	pyplot.show()
	pyplot.savefig(output, format='eps')
	print "finished plotting"

if __name__ == '__main__':
	AvgRate = [[3.78, 5.34, 5.78], [3.79, 7.37, 9.04], [3.78, 7.42, 9.22]]
	StdRate = [[0.9814, 0.687, 0.552], [0.00391, 0.83059, 0.30999], [0.06, 0.147, 0.239]]
	# NoVT = [[3.78, 5.34, 5.78], [0.9813981478, 0.687, 0.552]]
	# TDF = [[3.79, 7.37, 9.04], [0.00390964, 0.8305878486, 0.3099931124]]
	# Testbed = [[3.78, 7.42, 9.22], [0.06, 0.147, 0.239]]
	drawData('Perf40SwDiffBw')
