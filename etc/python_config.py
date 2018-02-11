import time
import sys
import os

from collections import defaultdict

NUM_RACKS = 8
HOSTS_PER_RACK = 16

TDF = 20.0

DATA_EXT_IF = 'enp8s0'
DATA_INT_IF = 'eth1'
DATA_NET = 1
DATA_RATE = 10 / TDF  # Gbps

CONTROL_EXT_IF = 'enp8s0d1'
CONTROL_INT_IF = 'eth2'
CONTROL_NET = 2
CONTROL_RATE = 10 / TDF  # Gbps

PHOST_IP = 100

SWITCH_CONTROL_IP = '10.%s.100.100' % CONTROL_NET
SWITCH_DATA_IP = '10.%s.100.100' % DATA_NET

CPU_COUNT = 16
CPU_SET = "1-%d" % (CPU_COUNT-1)  # Leave lcore 0 for IRQ
CPU_LIMIT = int((CPU_COUNT-1) * 100 / TDF)  # 75

TIMESTAMP = int(time.time())
SCRIPT = os.path.splitext(os.path.basename(sys.argv[0]))[0]
EXPERIMENTS = []
PHYSICAL_NODES = []

RPYC_PORT = 18861
RPYC_CONNECTIONS = {}

FQDN = 'etalon.local'

PACKET_BW = 10 * 10**9
CIRCUIT_BW = 80 * 10**9
CIRCUIT_BW_BY_TDF = 80 / TDF
PACKET_BW_BY_TDF = 10 / TDF

PACKET_LATENCY = 0.000100
CIRCUIT_LATENCY = 0.000600

RECONFIG_DELAY = 20

CONTROL_SOCKET_PORT = 1239

NODES_FILE = '../etc/handles'

# host commands
DOCKER_IMAGE = 'etalon'
DOCKER_CLEAN = 'sudo docker ps -q | xargs sudo docker stop -t 0 ' \
               '2> /dev/null; ' \
               'sudo docker ps -aq | xargs sudo docker rm 2> /dev/null'
DOCKER_BUILD = 'sudo docker build -t etalon -f ' \
               '/etalon/vhost/etalon.dockerfile /etalon/vhost/'
DOCKER_LOCAL_IMAGE_PATH = '/etalon/vhost/etalon.img'
DOCKER_SAVE = 'sudo docker save -o {img} etalon && sudo chown ' \
              '`whoami` {img}'.format(img=DOCKER_LOCAL_IMAGE_PATH)
DOCKER_REMOTE_IMAGE_PATH = '/etalon/vhost/etalon.img'
DOCKER_LOAD = 'sudo docker load -i %s' % DOCKER_REMOTE_IMAGE_PATH
DOCKER_RUN = 'sudo docker run -d -h h{id}.{FQDN} -v ' \
             '{hosts_file}:/etc/hosts:ro ' \
             '--cpuset-cpus={cpu_set} -c {cpu_limit} --name=h{id} '\
             '{image} {cmd}'
DOCKER_RUN_HDFS = 'sudo docker run -d -h h{id}.{FQDN} -v ' \
                  '{hosts_file}:/etc/hosts:ro ' \
                  '--mount=type=tmpfs,tmpfs-size=10G,destination=' \
                  '/usr/local/hadoop/hadoop_data/hdfs ' \
                  '--mount=type=tmpfs,tmpfs-size=128M,destination=' \
                  '/usr/local/hadoop/hadoop_data/hdfs-nn ' \
                  '--ulimit nofile=262144:262144 ' \
                  '--cpuset-cpus={cpu_set} -c {cpu_limit} --name=h{id} '\
                  '{image} {cmd}'
PIPEWORK = 'sudo pipework {ext_if} -i {int_if} h{rack}{id} ' \
           '10.{net}.{rack}.{id}/16; '
TC = 'sudo pipework tc h{id} qdisc add dev {int_if} root netem rate {rate}gbit'
SWITCH_PING = 'ping switch -c1'
GET_SWITCH_MAC = "arp | grep switch | tr -s ' ' | cut -d' ' -f3"
ARP_POISON = 'arp -s h{id} {switch_mac}'
SET_CC = 'sudo sysctl -w net.ipv4.tcp_congestion_control={cc}'

DID_BUILD_FN = '/tmp/docker_built'
HOSTS_FILE = '/tmp/hosts'
REMOVE_HOSTS_FILE = 'sudo rm %s' % (HOSTS_FILE)

DFSIOE = '/root/HiBench/bin/workloads/micro/dfsioe/hadoop/run_write.sh'
SCP = 'scp -r -o StrictHostKeyChecking=no root@%s:%s %s'
SCP_TO = 'scp -r -o StrictHostKeyChecking=no %s %s:%s'

# image commands
IMAGE_CPU = defaultdict(lambda: CPU_LIMIT, {
    'HDFS': (CPU_COUNT - 1) * 100,
    'reHDFS': (CPU_COUNT - 1) * 100,
})

IMAGE_SKIP_TC = defaultdict(lambda: False, {
    'HDFS': True,
    'HDFS_adu': True,
    'reHDFS': True,
    'reHDFS_adu': True,
})

IMAGE_NUM_HOSTS = defaultdict(lambda: HOSTS_PER_RACK, {
    'HDFS': 1,
    'HDFS_adu': 1,
    'reHDFS': 1,
    'reHDFS_adu': 1,
})

IMAGE_SETUP = {
    'flowgrindd': defaultdict(lambda: 'flowgrindd'),
    'flowgrindd_adu': defaultdict(lambda: 'flowgrindd_adu'),
    'HDFS': defaultdict(lambda: 'HDFS', {'h11': 'HDFS_nn'}),
    'HDFS_adu': defaultdict(lambda: 'HDFS_adu', {'h11': 'HDFS_nn_adu'}),
    'reHDFS': defaultdict(lambda: 'reHDFS', {'h11': 'reHDFS_nn'}),
    'reHDFS_adu': defaultdict(lambda: 'reHDFS_adu', {'h11': 'reHDFS_nn_adu'}),
}

IMAGE_DOCKER_RUN = defaultdict(lambda: DOCKER_RUN, {
    'HDFS': DOCKER_RUN_HDFS,
    'HDFS_adu': DOCKER_RUN_HDFS,
    'reHDFS': DOCKER_RUN_HDFS,
    'reHDFS_adu': DOCKER_RUN_HDFS,
})

IMAGE_CMD = {
    'flowgrindd': '"pipework --wait && pipework --wait -i eth2 && '
                  'LD_PRELOAD=libVT.so taskset -c {cpu} '
                  'flowgrindd -d -c {cpu}"',
    
    'flowgrindd_adu': '"pipework --wait && pipework --wait -i eth2 && '
                      'LD_PRELOAD=libVT.so:libADU.so taskset -c {cpu} '
                      'flowgrindd -d -c {cpu}"',
    
    'HDFS': '"service ssh start && '
            'pipework --wait && pipework --wait -i eth2 && sleep infinity"',

    'HDFS_adu': '"echo export LD_PRELOAD=libADU.so >> /root/.bashrc && '
                'export LD_PRELOAD=libADU.so && '
                'echo PermitUserEnvironment yes >> /etc/ssh/sshd_config && '
                'echo LD_PRELOAD=libADU.so > /root/.ssh/environment && '
                'service ssh start && '
                'pipework --wait && pipework --wait -i eth2 && '
                'sleep infinity"',

    'HDFS_nn': '"service ssh start && '
               'pipework --wait && pipework --wait -i eth2 && '
               '/usr/local/hadoop/bin/hdfs namenode -format -force && '
               '/tmp/config/start_hadoop.sh && sleep infinity"',

    'HDFS_nn_adu': '"echo export LD_PRELOAD=libADU.so >> /root/.bashrc && '
                   'export LD_PRELOAD=libADU.so && '
                   'echo PermitUserEnvironment yes >> /etc/ssh/sshd_config && '
                   'echo LD_PRELOAD=libADU.so > /root/.ssh/environment && '
                   'service ssh start && '
                   'pipework --wait && pipework --wait -i eth2 && '
                   '/usr/local/hadoop/bin/hdfs namenode -format -force && '
                   '/tmp/config/start_hadoop.sh && sleep infinity"',
}

# flowgrind
FLOWGRIND_PORT = 5999

# hdfs
HDFS_PORT = 50010  # datanode transfer port

# click
CLICK_ADDR = 'localhost'
CLICK_PORT = 1239
CLICK_BUFFER_SIZE = 1024


def handle_to_machine(h):
    machines = open(NODES_FILE, 'r').read().split('\n')[:-1]
    for machine in machines:
        handle, hostname = [m.strip() for m in machine.split('#')]
        if handle == h:
            return hostname
    return None


def get_phost_from_host(h):
    if h in PHYSICAL_NODES:
        return h
    if h[0] == 'h':
        return 'host%s' % h[1:2]
    return 'host%s' % h[0]


def get_phost_id(phost):
    return int(phost.split('host')[-1])


def get_phost_from_id(id):
    return 'host%d' % (id)


def get_host_from_rack_and_id(r, id):
    return 'h%d%d' % (r, id)


def get_rack_and_id_from_host(h):
    if h in PHYSICAL_NODES:
        return (PHOST_IP, get_phost_id(h))
    else:
        return int(h[1]), int(h[2])


def get_ip_from_host(h, net):
    r, s = get_rack_and_id_from_host(h)
    return '10.%s.%s.%s' % (net, r, s)


def get_data_ip_from_host(h):
    return get_ip_from_host(h, DATA_NET)


def get_control_ip_from_host(h):
    return get_ip_from_host(h, CONTROL_NET)


def gen_hosts_file(fn):
    fp = open(fn, 'w')
    fp.write('127.0.0.1\tlocalhost\n')
    fp.write('%s\tswitch\n' % (SWITCH_DATA_IP))
    for r in xrange(1, NUM_RACKS+1):
        for id in xrange(1, HOSTS_PER_RACK+1):
            fp.write('10.{net}.{rack}.{id}\th{rack}{id}.{fqdn}\n'.format(
                     net=DATA_NET, rack=r, id=id, fqdn=FQDN))
    fp.close()
