import time
import sys
import os

NUM_RACKS = 8
HOSTS_PER_RACK = 16
HADOOP_HOSTS_PER_RACK = 1

DATA_EXT_IF = 'enp8s0'
DATA_INT_IF = 'eth1'
DATA_NET = 1
DATA_RATE = 10 / TDF  # Gbps

CONTROL_EXT_IF = 'enp8s0d1'
CONTROL_INT_IF = 'eth2'
CONTROL_NET = 2
CONTROL_RATE = 10 / TDF  # Gbps

CPU_COUNT = 16
CPU_SET = "1-%d" % (CPU_COUNT-1)  # Leave lcore 0 for IRQ
CPU_LIMIT = int((CPU_COUNT-1) * 100 / TDF)  # 75

TDF = 20.0
TIMESTAMP = int(time.time())
SCRIPT = os.path.splitext(os.path.basename(sys.argv[0]))[0]
EXPERIMENTS = []
PHYSICAL_NODES = []

RPYC_PORT = 18861
RPYC_CONNECTIONS = {}

FQDN = 'h%d%d.etalon.local'

PACKET_BW = 10 * 10**9
CIRCUIT_BW = 80 * 10**9
CIRCUIT_BW_BY_TDF = 80 / TDF
PACKET_BW_BY_TDF = 10 / TDF

PACKET_LATENCY = 0.000100
CIRCUIT_LATENCY = 0.000600

RECONFIG_DELAY = 20

CONTROL_SOCKET_PORT = 1239

NODES_FILE = '/etalon/etc/handles.cloudlab'

# host commands
DOCKER_CLEAN = 'sudo docker ps -q | xargs sudo docker stop -t 0 ' \
               '2> /dev/null; ' \
               'sudo docker ps -aq | xargs sudo docker rm 2> /dev/null'
DOCKER_BUILD = 'sudo docker build -t etalon -f /etalon/vhost/etalon.dockerfile ' \
               '/etalon/vhost/'
DOCKER_RUN = 'sudo docker run -d -h h{id}.{FQDN} -v /etalon/vhost/config/hosts:/etc/hosts:ro ' \
             '--mount=type=tmpfs,tmpfs-size=10G,destination=/usr/local/hadoop/hadoop_data/hdfs ' \
             '--mount=type=tmpfs,tmpfs-size=128M,destination=/usr/local/hadoop/hadoop_data/hdfs-nn ' \
             '--ulimit nofile=262144:262144 ' \
             '--cpuset-cpus={cpu_set} -c {cpu_limit} --name=h{id} {image} {cmd}'
DOCKER_GET_PID = "sudo docker inspect --format '{{{{.State.Pid}}}}' h{id}"
DOCKER_EXEC = 'sudo docker exec -t h{id} {cmd}'
PIPEWORK = 'sudo pipework {ext_if} -i {int_if} h{rack}{id} ' \
           '10.{net}.{rack}.{id}/16; '
TC = 'sudo pipework tc h{id} qdisc add dev {int_if} root netem rate {rate}gbit'
NS_RUN = 'sudo nsenter -t {pid} -n {cmd}'
SWITCH_PING = 'ping switch -c1'
GET_SWITCH_MAC = "arp | grep switch | tr -s ' ' | cut -d' ' -f3"
ARP_POISON = 'arp -s h{id} {switch_mac}'

DFSIOE = '/root/HiBench/bin/workloads/micro/dfsioe/hadoop/run_write.sh'

# click
CLICK_ADDR = 'localhost'
CLICK_PORT = 1239
CLICK_BUFFER_SIZE = 1024


def handle_to_machine(h):
    machines = open(NODES_FILE, 'r').read().split('\n')[:-1]
    for machine in machines:
        handle, hostname = [m.strip() for m in machine.split('#')]
        if handle == h:
            return handle
    return None

