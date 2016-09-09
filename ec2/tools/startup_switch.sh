#!/bin/bash -ex
# This runs within the EC2 instance to do basic setup

# sudo without password
sed -Ei 's/^(Defaults.*requiretty)/#\1/' /etc/sudoers

# setup folders where code will live
mkdir -p /home/ec2-user/dc
chown ec2-user:ec2-user -R /home/ec2-user/dc

# enable ip forwarding and disable icmp redirects
echo "# Kernel sysctl configuration file for Red Hat Linux
#
# For binary values, 0 is disabled, 1 is enabled.  See sysctl(8) and
# sysctl.conf(5) for more details.

# Controls IP packet forwarding
net.ipv4.ip_forward = 1
net.ipv4.conf.default.send_redirects = 0
net.ipv4.conf.all.send_redirects = 0
net.ipv4.conf.eth0.send_redirects = 0
net.ipv4.conf.lo.send_redirects = 0

# Controls source route verification
net.ipv4.conf.default.rp_filter = 1

# Do not accept source routing
net.ipv4.conf.default.accept_source_route = 0

# Controls the System Request debugging functionality of the kernel
kernel.sysrq = 0

# Controls whether core dumps will append the PID to the core filename.
# Useful for debugging multi-threaded applications.
kernel.core_uses_pid = 1

# Controls the use of TCP syncookies
net.ipv4.tcp_syncookies = 1

# Controls the default maxmimum size of a mesage queue
kernel.msgmnb = 65536

# Controls the maximum size of a message, in bytes
kernel.msgmax = 65536

# Controls the maximum shared segment size, in bytes
kernel.shmmax = 68719476736

# Controls the maximum number of shared memory segments, in pages
kernel.shmall = 4294967296
" > /etc/sysctl.conf

sysctl -p


# get tools
yum -y upgrade
yum -y install emacs
yum -y --enablerepo=epel install iperf iperf3
yum -y install tcpdump
yum -y groupinstall "Development Tools"

cd /lib/modules/4.4.11-23.53.amzn1.x86_64/
unlink build
sudo ln -s ../../../usr/src/kernels/4.4.19-29.55.amzn1.x86_64 build
cd -

# fix netfilter
sed -i 's/limits\.h/linux\/kernel\.h/g' /usr/src/kernels/4.4.19-29.55.amzn1.x86_64/include/uapi/linux/netfilter_ipv4.h

echo "PATH=\$HOME/dc/tools:\$PATH" >> ~/.bash_profile

reboot

