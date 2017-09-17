#!/bin/bash

if [[ $EUID -ne 0 ]]; then
    echo "This script must be run as root"
    exit 1
fi

cd $HOME/sdrt/
OTHER_USER=`who | head -n1 | cut -f1 -d' '`
su - $OTHER_USER -c "cd sdrt; git pull"

sudo easy_install rpyc

if ! hostname | grep -q router
then
    h=`hostname | cut -d'.' -f1`
else
    h='host-1'
fi
sudo ifconfig eth2 10.10.1.`echo ${h:4} | awk '{print $1+2}'` netmask 255.255.255.0
sudo ifconfig eth3 10.10.2.`echo ${h:4} | awk '{print $1+2}'` netmask 255.255.255.0

sudo ifconfig eth2 mtu 9000
sudo ifconfig eth2 txqueuelen 10000

sudo ethtool -A eth2 rx on tx on
sudo ethtool -A eth3 rx off tx off

sudo ethtool -L eth2 rx 1 tx 1
sudo ethtool -L eth3 rx 1 tx 1
sudo ethtool -C eth2 adaptive-rx off
sudo ethtool -C eth3 adaptive-rx off

sudo sysctl -w net.core.netdev_max_backlog=250000
sudo sysctl -w net.core.optmem_max=16777216
sudo sysctl -w net.core.rmem_default=16777216
sudo sysctl -w net.core.wmem_default=16777216
sudo sysctl -w net.core.rmem_max=268435456
sudo sysctl -w net.core.wmem_max=268435456

# sudo sysctl -w net.ipv4.tcp_mem="374847 499797 749694"
# sudo sysctl -w net.ipv4.udp_mem="374847 499797 749694"
sudo sysctl -w net.ipv4.tcp_rmem="4096 87380 134217728"
sudo sysctl -w net.ipv4.tcp_wmem="4096 65536 134217728"

sudo sysctl -w net.ipv4.tcp_timestamps=1
sudo sysctl -w net.ipv4.tcp_sack=1
sudo sysctl -w net.ipv4.tcp_low_latency=0
sudo sysctl -w net.ipv4.tcp_mtu_probing=1
sudo sysctl -w net.ipv4.tcp_adv_win_scale=1
sudo sysctl -w net.ipv4.tcp_congestion_control=cubic

sudo sed -i -r 's/10.10.2/10.10.1/' /etc/hosts

for i in {1..8}
do
    for j in {1..8}
    do
	if ! grep -q "h$i$j" /etc/hosts
	then
	    printf "%s\t%s\n" "10.10.1.$i$j" "h$i$j" | sudo tee -a /etc/hosts
	fi
    done
done

if ! grep -q "apt.emulab" ~/.ssh/authorized_keys
then
    cat ~/.ssh/id_rsa.pub >> ~/.ssh/authorized_keys
fi

cd $HOME/sdrt/iputils/
make
sudo rm /usr/local/bin/ping 2>/dev/null
sudo ln -s $HOME/sdrt/iputils/ping /usr/local/bin/ping 2>/dev/null

$HOME/sdrt/cloudlab/kill.sh

cd $HOME/sdrt/sdrt-ctrl/lib
make

cd $HOME/sdrt/vt-mininet/mininet
sudo make install
sudo make install

ulimit -n 119353
ulimit -Hn 119353

# sudo ln -s $HOME/sdrt/iperf-2.0.10/src/iperf /usr/local/bin/iperf 2>/dev/null

# if ! sudo iptables -C PREROUTING -t mangle -p tcp --tcp-flags FIN,SYN,RST,ACK ACK -j TOS --set-tos Minimize-Delay
# then
#     sudo iptables -A PREROUTING -t mangle -p tcp --tcp-flags FIN,SYN,RST,ACK ACK -j TOS --set-tos Minimize-Delay
# fi

# sudo tc qdisc add dev eth0 root handle 1: prio
# sudo tc filter add dev eth2 protocol ip parent 1:0 prio 1 u32 match ip protocol 6 0xff match u8 0x05 0x0f at 0 match u16 0x0000 0xffc0 at 2 match u8 0x10 0xff at 33 flowid 1:1
