#!/bin/bash

LOG=iperf.out
exec > >(tee $LOG)

BRIDGE=ovs-test

ip netns exec ns2 iperf -c 10.1.1.1 -t 10 -f m -r

# delete tc on tap1 and tap2
ip netns exec ns1 tc qdisc del dev tap1 root
ip netns exec ns2 tc qdisc del dev tap2 root


# bring all ports down
ip netns exec ns1 ip link set dev tap1 down
ip netns exec ns2 ip link set dev tap2 down
ip link set dev ovs-tap1 down
ip link set dev ovs-tap2 down


# dispose veth pairs
ip netns exec ns1 ip link delete tap1
ip netns exec ns2 ip link delete tap2

# dispose Open vSwitch
ovs-vsctl del-port $BRIDGE ovs-tap1
ovs-vsctl del-port $BRIDGE ovs-tap2
ovs-vsctl del-br $BRIDGE

# delete namespace
ip netns delete ns1
ip netns delete ns2

# see if all things are cleaned up
# ip link show
# ovs-vsctl list-br
