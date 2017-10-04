#!/bin/bash

if [[ $EUID -ne 0 ]]; then
    echo "This script must be run as root"
    exit 1
fi

if ! hostname | grep -q router
then
    h=`hostname | cut -d'.' -f1`
else
    h='host-1'
fi
sudo ifconfig eth2 10.10.1.`echo ${h:4} | awk '{print $1+2}'` netmask 255.255.255.0
sudo ifconfig eth3 10.10.2.`echo ${h:4} | awk '{print $1+2}'` netmask 255.255.255.0

# install up-to-date OFED
# install vma --vma
# install vma --vma-eth
# added options mlx4_core enable_sys_tune=1 to mlx4.conf
# reload driver

sudo ifconfig eth2 mtu 9000
# sudo ifconfig eth2 txqueuelen 10000

sudo ethtool -A eth2 rx on tx on
sudo ethtool -A eth3 rx off tx off

sudo ethtool -L eth2 rx 1
sudo ethtool -L eth3 rx 1 tx 1

# sudo ethtool -C eth2 adaptive-rx off rx-usecs 0 rx-frames 0
# sudo ethtool -C eth2 adaptive-rx off
sudo ethtool -C eth3 adaptive-rx off

sudo ethtool -G eth2 rx 256

sudo ethtool -K eth2 lro on
sudo ethtool -K eth2 tx-nocache-copy off
# sudo ethtool -K eth2 ntuple on

# sudo sysctl -w net.core.netdev_max_backlog=1000
sudo sysctl -w net.core.optmem_max=4194304
sudo sysctl -w net.core.rmem_default=4194304
sudo sysctl -w net.core.wmem_default=4194304
sudo sysctl -w net.core.rmem_max=4194304
sudo sysctl -w net.core.wmem_max=4194304

sudo sysctl -w net.ipv4.tcp_rmem="4096 87380 4194304"
sudo sysctl -w net.ipv4.tcp_wmem="4096 65536 4194304"

sudo sysctl -w net.ipv4.tcp_timestamps=1
# sudo sysctl -w net.ipv4.tcp_low_latency=1
# sudo sysctl -w net.ipv4.tcp_mtu_probing=1
sudo sysctl -w net.ipv4.tcp_no_metrics_save=1

sudo service irqbalance stop
sudo set_irq_affinity_bynode.sh 0 eth2
sudo mlnx_tune -p HIGH_THROUGHPUT

# enable RPS
# cat /sys/class/net/eth2/device/local_cpus | sudo tee /sys/class/net/eth2/queues/rx-0/rps_cpus

# enable huge pages?
echo 1000000000 | sudo tee /proc/sys/kernel/shmmax
echo 800 | sudo tee /proc/sys/vm/nr_hugepages

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

if hostname | grep -q router
then
    sudo sed -i -r 's/10.10.1.([[:digit:]][[:digit:]])/10.10.2.\1/' /etc/hosts
fi


# have acks have own traffic class
# if ! sudo iptables -C PREROUTING -t mangle -p tcp --tcp-flags FIN,SYN,RST,ACK ACK -j TOS --set-tos Minimize-Delay
# then
#     sudo iptables -A PREROUTING -t mangle -p tcp --tcp-flags FIN,SYN,RST,ACK ACK -j TOS --set-tos Minimize-Delay
# fi

# sudo tc qdisc add dev eth0 root handle 1: prio
# sudo tc filter add dev eth2 protocol ip parent 1:0 prio 1 u32 match ip protocol 6 0xff match u8 0x05 0x0f at 0 match u16 0x0000 0xffc0 at 2 match u8 0x10 0xff at 33 flowid 1:1
