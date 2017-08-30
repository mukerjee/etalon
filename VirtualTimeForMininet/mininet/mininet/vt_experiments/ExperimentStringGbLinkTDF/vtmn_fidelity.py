#!/usr/bin/python

"""
Benchmarking experiment for fidelity

Test bandwidth (using iperf) on string/chain networks of fixed size 40,
using kernel datapaths.

First construct a network of 2 hosts and N switches, connected as follows:

       h1 - s1 - s2 - ... - sN - h2


Varying link bw, with & without virtual time, test throughput between h1 and h2
"""

import sys
import numpy
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

class StringTestTopo(Topo):
    "Topology for a string of N switches and 2 hosts."

    def __init__(self, N, **params):

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

    net = Mininet(topo=topo_class, host=host_class, switch=OVSKernelSwitch, controller=controller_class, waitConnected=True, link=link_class)
    # no tdf_adaptor to change TDF
    net.start()

    print "*** testing basic connectivity\n"
    src, dst = net.hosts
    if tdf == 1:
        num_pings = 3
        for i in irange(1, num_pings):
            ping_result = list(net.pingFull( [ src, dst ] ))
            # ping_result=[(host1), (host2)]
            # host = (src, dst, data)
            # data = (#sent, #received, rttmin, rttavg, rttmax, rttdev)
            print "Ping avg rtt = %s\n" % ping_result[0][2][3]
            rttavg = ping_result[0][2][3]
        data_file.write( "RTT Avg = %s ms\n" % rttavg)
    else:
        net.ping( [src, dst] )

    print "*** testing bandwidth\n"
    num_rounds = 5
    client_history = []
    time = 16
    omit = 1
    for i in irange(1, num_rounds):
        # bandwidth = net.iperf( [src, dst], l4Type = 'UDP', udpBw='%sM'%set_bw, format = 'm', time=20, clifile=data_file, serfile=data_file )
        bandwidth = net.iperf( [src, dst], l4Type = 'TCP', format = 'm', time=time, omit=omit, clifile=data_file, serfile=data_file )
        flush()
        serout = bandwidth[0]
        cliout = bandwidth[1]

        if len(serout) > 0 and len(cliout) > 0:
            serDataStr, unit = serout.split(" ")
            serData = float(serDataStr)

            cliDataStr, unit = cliout.split(" ")
            cliData = float(cliDataStr)
            client_history.append(cliData)
            data_file.write("%s\t%f\t%f\t%s\t%s\n" % (size, src.tdf, net.cpu_usage, serData, cliData))

    client_mean = numpy.mean(client_history)
    client_stdev = numpy.std(client_history)
    data_file.write( "Avg Throughtput = %f\n" % client_mean )
    data_file.write( "STD Throughput = %f\n" % client_stdev )
    print "AVG = %f " % client_mean
    print "STD = %f " % client_stdev
    data_file.write('\n\n')
    net.stop()
    return client_mean, client_stdev

def runTest(file_name, controller, tdf, size, set_cpu, set_bw, set_delay="10us"):
    lg.setLogLevel( 'info' )

    """in fact, Controller and Remotecontroller have no difference
    all we need to do is start or not start POX in another shell"""
    if controller == "POX":
        controller = partial( RemoteController, ip = '127.0.0.1', port=6633 )
    else:
        controller = DefaultController
    link = partial( TCLink, bw=set_bw, delay=set_delay )

    """config host's cpu share and time dilation factor"""
    host = custom(CPULimitedHost, sched='cfs', period_us=100000, cpu=set_cpu, tdf=tdf)

    """with w option, it automatically overwrite everytime"""
    data_file = open('%s.log' % file_name, 'w')
    print "Results are written to %s.log file" % file_name
    data_file.write("********* Running stringBandwidthTest *********\n")
    data_file.flush()

    # seems mininet cannot handle more than 640 switches
    print "******* Running with %d switches, TDF = %d *******" % (size, tdf)
    client_avg, client_stdev = stringBandwidthTest(host, controller, link, size, tdf, data_file)
    cleanup()
    return client_avg, client_stdev


def drawData(output, AvgRates, StdRates, BWs):
    BWsInGb = [ str(x/1000) for x in BWs]
    base_category = tuple(range(1, len(BWsInGb) + 1 ))
    dataLables = ['Mininet, TDF=1', 'Mininet, TDF=4', 'Physical Testbed']
    xLabel = 'Link Bandwidth (Gbps)'
    yLabel = 'Average TCP Throughput (Gbps)'

    color_list = ['c', 'r', 'm', 'y', 'g', 'b', 'k', 'w']
    hatch_list = ['/', '\\', '+', 'x', 'o', '.', '*', '-']
    width = 0.25
    fontSize = 14
    maxY = max(BWs) / 1000

    rects = []
    fig, ax = pyplot.subplots()
    for index in range(0, len(AvgRates)):
        category = [x + index * width for x in base_category]
        rect = ax.bar(category, AvgRates[index], width, color=color_list[index], yerr=StdRates[index], hatch=hatch_list[index])
        rects.append(rect)

    ax.legend(tuple(rects), dataLables, shadow=True, fancybox=True, fontsize=fontSize, loc='upper left')
    ax.set_xticks([x + width*3/2 for x in base_category ])
    # ax.set_xticklabels(('4', '8', '10'))
    ax.set_xticklabels(BWsInGb)
    ax.set_yticks(range(maxY+1))

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
    print "finished plotting"


def main():
    AvgRates = []
    StdRates = []
    TDFs = [1, 4]
    BWs = [2000, 3000, 4000, 5000]
    size = 12
    for tdf in TDFs:
        avg_rates = []
        std_rates = []
        for bw in BWs:
            file_name = "PerfStringBW%dMTDF%d" %(bw, tdf)
            avg, std = runTest(file_name, "NO", tdf, size, 0.5, bw)
            # convert to Gbps
            avg_rates.append(avg / 1024)
            std_rates.append(std / 1024)
        AvgRates.append(avg_rates)
        StdRates.append(std_rates)

    # trust me, I got them from physical testbed
    testbed_avg_rates = [3.78, 7.42, 9.22]
    testbed_std_rates = [0.06, 0.147, 0.239]
    ideal_avg_rates = [x / 1000 for x in BWs]
    ideal_std_rates = [x - x for x in BWs]
    AvgRates.append(ideal_avg_rates)
    StdRates.append(ideal_std_rates)

    print AvgRates
    print StdRates

    drawData('Perf%dSwDiffBw.eps' % size, AvgRates, StdRates, BWs)

if __name__ == '__main__':
    main()

