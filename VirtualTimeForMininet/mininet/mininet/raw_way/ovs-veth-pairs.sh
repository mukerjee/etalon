#!/bin/bash

# add two network namespace
ip netns add ns1
ip netns add ns2

# create Open vSwitch
BRIDGE=ovs-test
ovs-vsctl add-br $BRIDGE

# PORT 1
# create a port pair (tap1, ovs-tap1)
ip link add tap1 type veth peer name ovs-tap1
# attach one side to namespace 1
ip link set tap1 netns ns1
# attach other side to ovs
ovs-vsctl add-port $BRIDGE ovs-tap1
# bring port up
ip netns exec ns1 ip link set dev tap1 up
ip link set dev ovs-tap1 up

# PORT 2
# create a port pair (tap2, ovs-tap2)
ip link add tap2 type veth peer name ovs-tap2
# attach one side to namespace 2
ip link set tap2 netns ns2
# attach other side to ovs
ovs-vsctl add-port $BRIDGE ovs-tap2
# bring port up
ip netns exec ns2 ip link set dev tap2 up
ip link set dev ovs-tap2 up


# eth0      Link encap:Ethernet  HWaddr 34:17:eb:ca:c0:76  
#           inet addr:216.47.142.127  Bcast:216.47.142.255  Mask:255.255.255.0
#           inet6 addr: 2620:f3:8000:5002::e312/128 Scope:Global
#           inet6 addr: fe80::3617:ebff:feca:c076/64 Scope:Link
#           UP BROADCAST RUNNING MULTICAST  MTU:1500  Metric:1
#           RX packets:7248 errors:0 dropped:0 overruns:0 frame:0
#           TX packets:506 errors:0 dropped:0 overruns:0 carrier:0
#           collisions:0 txqueuelen:1000 
#           RX bytes:950077 (950.0 KB)  TX bytes:98245 (98.2 KB)

# lo        Link encap:Local Loopback  
#           inet addr:127.0.0.1  Mask:255.0.0.0
#           inet6 addr: ::1/128 Scope:Host
#           UP LOOPBACK RUNNING  MTU:65536  Metric:1
#           RX packets:838 errors:0 dropped:0 overruns:0 frame:0
#           TX packets:838 errors:0 dropped:0 overruns:0 carrier:0
#           collisions:0 txqueuelen:0 
#           RX bytes:240008 (240.0 KB)  TX bytes:240008 (240.0 KB)

# ovs-tap1  Link encap:Ethernet  HWaddr 2e:6e:d5:d9:3f:3b  
#           inet6 addr: fe80::2c6e:d5ff:fed9:3f3b/64 Scope:Link
#           UP BROADCAST RUNNING MULTICAST  MTU:1500  Metric:1
#           RX packets:34 errors:0 dropped:0 overruns:0 frame:0
#           TX packets:109 errors:0 dropped:0 overruns:0 carrier:0
#           collisions:0 txqueuelen:1000 
#           RX bytes:5852 (5.8 KB)  TX bytes:19555 (19.5 KB)

# ovs-tap2  Link encap:Ethernet  HWaddr 82:be:c6:20:a1:8d  
#           inet6 addr: fe80::80be:c6ff:fe20:a18d/64 Scope:Link
#           UP BROADCAST RUNNING MULTICAST  MTU:1500  Metric:1
#           RX packets:35 errors:0 dropped:0 overruns:0 frame:0
#           TX packets:56 errors:0 dropped:0 overruns:0 carrier:0
#           collisions:0 txqueuelen:1000 
#           RX bytes:5601 (5.6 KB)  TX bytes:10503 (10.5 KB)

# ovs-test  Link encap:Ethernet  HWaddr 3e:e6:2e:06:cd:4b  
#           inet6 addr: fe80::8477:ff:fede:e91d/64 Scope:Link
#           UP BROADCAST RUNNING  MTU:1500  Metric:1
#           RX packets:16 errors:0 dropped:0 overruns:0 frame:0
#           TX packets:8 errors:0 dropped:0 overruns:0 carrier:0
#           collisions:0 txqueuelen:0 
#           RX bytes:1296 (1.2 KB)  TX bytes:648 (648.0 B)


# assign ip address to tap1 and tap2
ip netns exec ns1 ip addr add 10.1.1.1/24 dev tap1
ip netns exec ns2 ip addr add 10.1.1.2/24 dev tap2

# add tc to tap1
ip netns exec ns1 tc qdisc add dev tap1 root handle 1: htb default 12 
ip netns exec ns1 tc class add dev tap1 parent 1:1 classid 1:12 htb rate 100mbit burst 15k
ip netns exec ns1 tc qdisc add dev tap1 parent 1:12 netem delay 1ms

ip netns exec ns1 ip link show
# 7: tap1: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc htb state UP mode DEFAULT group default qlen 1000
#     link/ether 4e:54:6c:cf:7d:ae brd ff:ff:ff:ff:ff:ff

# add tc to ovs-tap1
#tc qdisc add dev ovs-tap1 root handle 1: htb default 12 
#tc class add dev ovs-tap1 parent 1:1 classid 1:12 htb rate 1000mbit burst 15k
#tc qdisc add dev ovs-tap1 parent 1:12 netem delay 200ms
#tc qdisc show dev ovs-tap1

# add tc to tap2
ip netns exec ns2 tc qdisc add dev tap2 root handle 1: htb default 12 
ip netns exec ns2 tc class add dev tap2 parent 1:1 classid 1:12 htb rate 100mbit burst 15k
ip netns exec ns2 tc qdisc add dev tap2 parent 1:12 netem delay 1ms

ip netns exec ns2 ip link show
# 9: tap2: <BROADCAST,MULTICAST,UP,LOWER_UP> mtu 1500 qdisc htb state UP mode DEFAULT group default qlen 1000
#     link/ether 26:91:c1:4d:fd:b0 brd ff:ff:ff:ff:ff:ff

# add tc to ovs-tap2
#tc qdisc add dev ovs-tap2 root handle 1: htb default 12 
#tc class add dev ovs-tap2 parent 1:1 classid 1:12 htb rate 1000mbit burst 15k
#tc qdisc add dev ovs-tap2 parent 1:12 netem delay 200ms
#tc qdisc show dev ovs-tap2

# iperf between tap1 and tap2
sudo killall iperf

ip netns exec ns1 iperf -s -f m >> server.out
IPERF_SERVER_PID=$!
echo $IPERF_SERVER_PID



