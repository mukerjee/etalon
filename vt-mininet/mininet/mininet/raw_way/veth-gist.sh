#!/bin/sh
set -ex
which iperf >/dev/null || apt-get install iperf
mkdir -p /var/run/netns
 
unshare --net -- sh -c "sleep 3; exec iperf -s" &
PID1=$!
ln -sf /proc/$PID1/ns/net /var/run/netns/ns1 
 
unshare --net -- sh -c "sleep 3; exec iperf -c 10.1.1.1 -i 1" &
PID2=$!
ln -sf /proc/$PID2/ns/net /var/run/netns/ns2
 
ip link add e1 type veth peer name e2
for N in 1 2
do
  ip link set e$N netns ns$N
  ip netns exec ns$N ip link set lo up
  ip netns exec ns$N ip link set e$N up
  ip netns exec ns$N ip addr add 10.1.1.$N/24 dev e$N
done
 
wait $PID2
kill $PID1