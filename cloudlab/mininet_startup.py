#!/usr/bin/python

import subprocess
import os
from socket import gethostname

from mininet.net import Mininet
from mininet.node import HostWithPrivateDirsCPULimited
from mininet.cli import CLI
from mininet.link import Intf, Link, TCLink, TCIntf
from mininet.log import setLogLevel, info
from mininet.util import waitListening

host = int(gethostname().split('.')[0][4:])
TDF = 20.0
CIRCUIT_LINK = 80000  # Mbps
PACKET_LINK = 10000  # Mbps
#TDF = 1.0
#CIRCUIT_LINK = 4000  # Mbps
#PACKET_LINK = 500  # Mbps
NUM_RACKS = 8
HOSTS_PER_RACK = 8

def myNetwork():
    net = Mininet(topo=None, build=False)

    info('*** Adding controller\n')
    net.addController(name='c0')

    info('*** Add switches\n')
    s1 = net.addSwitch('s1')
    s1_eth2 = TCIntf('eth2', node=s1, bw=CIRCUIT_LINK / TDF)

    s2 = net.addSwitch('s2')
    s2_eth3 = Intf('eth3', node=s2)

    info('*** Add hosts and links\n')
    hosts = []
    for i in xrange(HOSTS_PER_RACK):
        j = (host+1, i+1)
        hosts.append(net.addHost('h%d%d' % j, ip='10.10.1.%d%d/24' % j,
                                 cls=HostWithPrivateDirsCPULimited,
				 sched='cfs', period_us=100000,
                                 privateDirs = ['/hadoop'],
				 tdf=TDF))
        hosts[-1].setCPUFrac(1.0 / TDF)

        l = TCLink(hosts[i], s1, intfName1='h%d%d-eth1' % j, intfName2='s1-h%d%d-eth1' % j,  bw=PACKET_LINK / TDF)
        l.intf1.setMAC('AA:AA:AA:AA:AA:%d%d' % j)
        l = Link(hosts[i], s2, intfName1='h%d%d-eth2' % j, intfName2='s2-h%d%d-eth2' % j)
        l.intf1.setIP('10.10.2.%d%d/24' % j)
        hosts[-1].cmd("ifconfig h%d%d-eth1 mtu 9000" % j)
        hosts[-1].cmd("hostname h%d%d" % j)
        s1.cmd("ifconfig s1-h%d%d-eth1 mtu 9000" % j)

    info('*** Starting network\n')
    net.start()

    info('*** setting up host arp poisoning\n')
    hosts[0].cmd('ping router -c1')
    router_mac = hosts[0].cmd("arp | grep router | tr -s ' ' | cut -d' ' -f3")
    for h in hosts:
        for i in xrange(NUM_RACKS):
            for j in xrange(HOSTS_PER_RACK):
                h.cmd("arp -s 10.10.1.%d%d %s" % (i+1, j+1, router_mac))

    info('*** launching iperf daemon\n')
    for h in hosts:
        # h.cmd("iperf3 -s -D &")
        h.cmd("./rpyc_daemon.py &")
        for i in xrange(NUM_RACKS):
            for j in xrange(HOSTS_PER_RACK):
                h.cmd("iperf3 -p53%d%d -s -D &" % (i+1, j+1))

    info('*** launching sshd\n')
    for h in hosts:
        h.cmd('/usr/sbin/sshd -D &')

    CLI(net)
    net.stop()

if __name__ == '__main__':
    subprocess.call([os.path.expanduser('~/sdrt/cloudlab/tune.sh')])
    setLogLevel('info')
    myNetwork()
