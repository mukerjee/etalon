#!/usr/bin/env python

from config import elastic_ip

fn = 'startup_hosts.sh'

data = """#!/bin/bash -ex
# This runs within the EC2 instance to do basic setup

# sudo without password
sed -Ei 's/^(Defaults.*requiretty)/#\1/' /etc/sudoers

# setup folders where code will live
mkdir -p /home/ec2-user/dc
chown ec2-user:ec2-user -R /home/ec2-user/dc
# Setting Hadoop
wget http://apache.mesi.com.ar/hadoop/common/hadoop-2.7.3/hadoop-2.7.3.tar.gz -P /home/ec2-user/
tar xvzf /home/ec2-user/hadoop-2.7.3.tar.gz -C /home/ec2-user/

sudo mkdir /usr/local/hadoop
sudo mv /home/ec2-user/hadoop-2.7.3/* /usr/local/hadoop/
sudo chown -R ec2-user /usr/local/hadoop

echo "export JAVA_HOME=/usr/lib/jvm/java-1.7.0-openjdk-1.7.0.121.x86_64
export HADOOP_INSTALL=/usr/local/hadoop
export PATH=$PATH:\$HADOOP_INSTALL/bin
export PATH=$PATH:\$HADOOP_INSTALL/sbin
export HADOOP_MAPRED_HOME=\$HADOOP_INSTALL
export HADOOP_COMMON_HOME=\$HADOOP_INSTALL
export HADOOP_HDFS_HOME=\$HADOOP_INSTALL
export YARN_HOME=\$HADOOP_INSTALL
export HADOOP_COMMON_LIB_NATIVE_DIR=\$HADOOP_INSTALL/lib/native
export HADOOP_OPTS=\"-Djava.library.path=$HADOOP_INSTALL/lib\"
export HADOOP_CONF_DIR=/usr/local/hadoop/etc/hadoop" >> /home/ec2-user/.bash_profile

source /home/ec2-user/.bash_profile

echo "export JAVA_HOME=/usr/lib/jvm/java-1.7.0-openjdk-1.7.0.121.x86_64" >> $HADOOP_CONF_DIR/hadoop-env.sh

# add static route through switch
SUBNET="172.31.16.0/20"
ELASTIC_IP_DNS="%s"

echo "#!/bin/bash
sudo ip route del" $SUBNET "
sudo ip route add \`getent hosts" $ELASTIC_IP_DNS "| cut -f1 -d' '\` dev eth0
getent hosts" $ELASTIC_IP_DNS "| cut -f1 -d' ' | sudo xargs ip route add" $SUBNET "via" > /etc/init.d/add_route

chmod +x /etc/init.d/add_route
ln -s /etc/init.d/add_route /etc/rc3.d/S91add-route
/etc/init.d/add_route

# get tools
yum -y upgrade
yum -y install emacs
yum -y --enablerepo=epel install iperf iperf3
yum -y install tcpdump
yum -y install java-1.7.0-openjdk-devel.x86_64
yum -y install python2
yum -y install htop
yum -y groupinstall "Development Tools"

# fix netfilter
#sed -i 's/limits\.h/linux\/kernel\.h/g' /usr/src/kernels/4.4.19-29.55.amzn1.x86_64/include/uapi/linux/netfilter_ipv4.h

echo "PATH=~/dc/tools:\$PATH" >> /home/ec2-user/.bash_profile
#echo "/etc/init.d/add_route" >> /home/ec2-user/.bash_profile


reboot

"""


def read():
    return data % elastic_ip


def dump_file():
    open("/tmp/" + fn, "w").write(data % elastic_ip)
    return "/tmp/" + fn
