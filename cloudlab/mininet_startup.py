#!/usr/bin/python

# import subprocess
# import os
from socket import gethostname

from mininet.net import Mininet
from mininet.node import CPULimitedHost
from mininet.cli import CLI
from mininet.link import Intf, Link
from mininet.log import setLogLevel, info

host = int(gethostname().split('.')[0][4:])

def myNetwork():
    net = Mininet(topo=None, build=False)

    info('*** Adding controller\n')
    net.addController(name='c0')

    info('*** Add switches\n')
    s1 = net.addSwitch('s1')
    Intf('eth1', node=s1)
    s2 = net.addSwitch('s2')
    Intf('eth2', node=s2)

    info('*** Add hosts\n')
    h = net.addHost('h', ip='10.10.1.%d' % (host+2), cls=CPULimitedHost,
                    sched='cfs', period_us=100000, cpu=1,
                    tdf=10)
    
    info('*** Add links\n')
    Link(h, s1, intfName1='h-eth1')
    Link(h, s2, intfName2='h-eth2')
    h.cmd('ifconfig h-eth2 10.10.2.%d netmask 255.255.255.0' % (host+2))

    info('*** Starting network\n')
    net.start()
    
    for i in xrange(16):
        h.cmd('ping 10.10.1.%d -c1 -W1' % (i+2));
    h.cmd('ping router -c1 -W1')

    for i in xrange(16):
        h.cmd('arp -s 10.10.1.%d `arp | grep router | tr -s ' ' | cut -d' ' -f3`'
              % (i+2))

    CLI(net)
    net.stop()

if __name__ == '__main__':
    # subprocess.call([os.path.expanduser('~/sdrt/cloudlab/node_arp_poison.sh')])
    setLogLevel('info')    
    myNetwork()
