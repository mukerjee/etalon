#!/usr/bin/python

"""
Test bandwidth (using iperf) on string/chain networks of varying size,
using both kernel and user datapaths.

We construct a network of 2 hosts and N switches, connected as follows:

       h1 - s1 - s2 - ... - sN - h2


WARNING: by default, the reference controller only supports 16
switches, so this test WILL NOT WORK unless you have recompiled
your controller to support 100 switches (or more.)

In addition to testing the bandwidth across varying numbers
of switches, this example demonstrates:

- creating a custom topology, StringTestTopo
- using the ping() and iperf() tests from Mininet()
- testing both the kernel and user switches

"""
import sys
import numpy
import timeit
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as pyplot

from mininet.net import Mininet
from mininet.node import *
from mininet.topo import Topo
from mininet.log import lg
from mininet.util import irange, custom
from mininet.node import CPULimitedHost
from mininet.link import TCLink
from functools import partial
from mininet.clean import cleanup

flush = sys.stdout.flush

class StringTestTopo( Topo ):
    "Topology for a string of N switches and 2 hosts."

    def __init__( self, N, **params ):

        # Initialize topology
        Topo.__init__( self, **params )

        # Create switches and hosts
        hosts = [ self.addHost( 'h%s' % h ) for h in irange( 1, 2 ) ]

        switches = [self.addSwitch( 's%s' % s ) for s in irange(1, N)]

        # Wire up hosts with switch
        self.addLink(hosts[0], switches[0])
        self.addLink(hosts[1], switches[ N - 1 ])

        last = None
        for switch in switches:
            if last:
                self.addLink(last, switch)
            last = switch


def stringBandwidthTest(host_class, controller_class, link_class, size, tdf, data_file):
    "Check bandwidth at various lengths along a switch chain."

    topo_class = StringTestTopo(size)

    net = Mininet( topo=topo_class, host=host_class, switch=OVSKernelSwitch, controller=controller_class, waitConnected=True, link=link_class)
    net.start()

    src, dst = net.hosts[ 0 ], net.hosts[ 1 ]
    print "*** testing basic connectivity"
    net.ping( [ src, dst ] )

    if tdf == 1:
        num_pings = 3
        for i in irange(1, num_pings):
            ping_result = list(net.pingFull( [src, dst] ))
            rttavg = ping_result[0][2][3]
        data_file.write("%s\t" % rttavg)
        data_file.flush()

    print "*** testing bandwidth"
    print "testing", src, "<->", dst

    num_rounds = 8
    #cli_results = []
    ser_results = []
    omit = 1
    time = 11
    start = timeit.default_timer()
    for i in irange(1, num_rounds):
        bandwidth = net.iperf([src, dst], l4Type = 'TCP',  format = 'm', time=time, omit=omit)
        flush()
        serout = bandwidth[0]
        cliout = bandwidth[1]

        if len(serout) > 0:
            serDataStr, unit = serout.split(" ")
            serData = float(serDataStr)
            if serData > 0.1:
                ser_results.append(serData)

        #if len(cliout) > 0:
            #cliDataStr, unit = cliout.split(" ")
            #cliData = float(cliDataStr)
            #if cliData > 0.1:
                #cli_results.append( cliData )

    end = timeit.default_timer()
    elapsed = end - start
    print "elapsed: %s\n" % elapsed
    # unit = Mbits/sec
    #avgCliBw = numpy.mean( cli_results )
    #stdevCliBw = numpy.std( cli_results )
    maxSerBw = numpy.amax(ser_results)
    minSerBw = numpy.amin(ser_results)
    if len(ser_results) >= 10:
        ser_results.remove(maxSerBw)
        ser_results.remove(minSerBw)

    avgSerBw = numpy.mean(ser_results)
    stdevSerBw = numpy.std(ser_results)
    print "Avg = %f" % avgSerBw
    print "Std = %f" % stdevSerBw
    #data_file.write("%s\t%s\t%s\t%s\t%s\n" % (size, avgCliBw, stdevCliBw, maxCliBw, minCliBw))
    data_file.write("%s\t%s\t%s\t%s\n" % (size, avgSerBw, stdevSerBw, elapsed))
    data_file.flush()
    net.stop()
    return avgSerBw, stdevSerBw

def runTest(file_name, controller, tdf, size, set_cpu, set_bw, set_delay="50us"):
    lg.setLogLevel( 'info' )

    """in fact, Controller and Remotecontroller have no difference
    all we need to do is start or not start POX in another shell"""
    if controller == "POX":
        controller = partial(RemoteController, ip='127.0.0.1', port=6633)
    else:
        controller = DefaultController
    link = partial(TCLink, bw=set_bw, delay=set_delay)

    """config host's cpu share and time dilation factor"""
    host = custom(CPULimitedHost, sched='cfs', period_us=100000, cpu=set_cpu, tdf=tdf)

    """with w option, it automatically overwrite everytime"""
    data_file = open('%s.log' % file_name, 'w')
    print "Results are written to %s.log file" % file_name

    data_file.write("********* Server String Topowith TDF = %d *********\n" % tdf)

    if tdf == 1:
        data_file.write("RTT\tSwitchCount\tAvg\tStd\tTime\n")
    else:
        data_file.write("SwitchCount\tAvg\tStd\tTime\n")
    data_file.flush()

    print "********* Running stringBandwidthTest *********", size
    avg, std = stringBandwidthTest(host, controller, link, size, tdf, data_file)

    cleanup()
    return avg, std

def drawData(output, AvgRates, StdRates, SwSizes, BW):
    base_category = tuple(range(1, len(SwSizes) + 1 ))
    dataLables = ['Mininet, TDF=1', 'Mininet, TDF=4', 'Physical Testbed']
    xLabel = '#Switches'
    yLabel = 'Average TCP Throughput (Gbps)'

    color_list = ['c', 'r', 'm', 'y', 'g', 'b', 'k', 'w']
    hatch_list = ['/', '\\', '+', 'x', 'o', '.', '*', '-']
    width = 0.25
    fontSize = 12
    maxY = BW

    rects = []
    fig, ax = pyplot.subplots()
    for index in range(0, len(AvgRates) ):
        category = [x + index * width for x in base_category]
        print category
        rect = ax.bar(category, AvgRates[index], width, color=color_list[index], yerr=StdRates[index], hatch=hatch_list[index])
        rects.append(rect)

    ax.legend(tuple(rects), dataLables, shadow=True, fancybox=True, fontsize=fontSize, loc='upper left')
    ax.set_xticks([x + width*3/2 for x in base_category ])
    # ax.set_xticklabels(('4', '8', '10'))
    ax.set_xticklabels(SwSizes)
    ax.set_yticks(range( maxY + 1 ))

    for tick in ax.xaxis.get_major_ticks():
        tick.label.set_fontsize(fontSize)
    for tick in ax.yaxis.get_major_ticks():
        tick.label.set_fontsize(fontSize)

    pyplot.grid()
    pyplot.ylim((0, maxY))
    pyplot.xlim(0.65, len(base_category) + 1)
    pyplot.xlabel(xLabel, fontsize=fontSize)
    pyplot.ylabel(yLabel, fontsize=fontSize)
    # pyplot.yticks([x for x in range(0, 1, 11)])
    # pyplot.xticks([x for x in range(1, 4, 1)])
    # pyplot.show()
    pyplot.savefig(output, format='eps')
    print "finished plotting, see %s" % output

def main():
    AvgRates = []
    StdRates = []
    TDFs = [1, 4]
    Sizes = [10, 20, 30, 40, 50]
    BW = 2000
    for tdf in TDFs:
        avg_rates = []
        std_rates = []
        for size in Sizes:
            file_name = "ScaleStringSw%dTDF%d" %(size, tdf)
            avg, std = runTest(file_name, "NO", tdf, size, 0.5, BW)
            # convert to Gbps
            avg_rates.append(avg / 1024)
            std_rates.append(std / 1024)
        AvgRates.append(avg_rates)
        StdRates.append(std_rates)

    Sizes2Str = [str(x) for x in Sizes]
    print AvgRates
    print StdRates

    drawData('ScaleBw%dDiffSz.eps' % BW, AvgRates, StdRates, tuple(Sizes2Str), BW / 1000)

if __name__ == '__main__':
    main()

