
import time
import sys
import os

from collections import defaultdict

NUM_RACKS = 3
HOSTS_PER_RACK = 16

TDF = 20.0

# data network
DATA_EXT_IF = 'enp68s0'  # As seen by physical nodes.
DATA_INT_IF = 'eth1'  # As seen by containers.
DATA_NET = 1
DATA_RATE = 10 / TDF  # Gbps

# control network
CONTROL_EXT_IF = 'eno4'  # As seen by physical nodes.
CONTROL_INT_IF = 'eth2'  # As seen by containers.
CONTROL_NET = 2
CONTROL_RATE = 10 / TDF  # Gbps

# 10.x.PHOST_IP.y
PHOST_IP = 100

# switch IPs
SWITCH_CONTROL_IP = '10.%s.%s.100' % (CONTROL_NET, PHOST_IP)
SWITCH_DATA_IP = '10.%s.%s.100' % (DATA_NET, PHOST_IP)

# cpu settings
CPU_COUNT = 40  # vcores per physical host
CPU_SET = "1-%d" % (CPU_COUNT-1)  # Leave lcore 0 for IRQ
CPU_LIMIT = int((CPU_COUNT-1) * 100 / TDF)  # 75

# experiment setup
TIMESTAMP = int(time.time())
SCRIPT = os.path.splitext(os.path.basename(sys.argv[0]))[0]
EXPERIMENTS = []
PHYSICAL_NODES = []

# rpyc
RPYC_PORT = 18861
RPYC_CONNECTIONS = {}

# e.g., h12.etalon.local
FQDN = 'etalon.local'

# link parameters
CIRCUIT_BW = 80 * 10**9
CIRCUIT_BW_BY_TDF = 80 / TDF
CIRCUIT_LATENCY = 0.000600
PACKET_BW = 10 * 10**9
PACKET_BW_BY_TDF = 10 / TDF
PACKET_LATENCY = 0.000100

# reconfiguration penalty
RECONFIG_DELAY = 20

# where are handles defined
NODES_FILE = '../etc/handles'

# flowgrind
FLOWGRIND_PORT = 5999

# hdfs
HDFS_PORT = 50010  # datanode transfer port

# software switch settings
CLICK_ADDR = 'localhost'
CLICK_PORT = 1239
CLICK_BUFFER_SIZE = 1024

# Default circuit schedule. Used when config type == "circuit".
#             src hosts:  1/2/3
#             dst hosts:  2/3/1
#              src idxs:  0/1/2
#              dst idxs:  1/2/0
DEFAULT_CIRCUIT_CONFIG = '1/2/0'

# The default packet size for flowgrind, in bytes.
DEFAULT_REQUEST_SIZE = 8948

# The queue size, in packets, beyond which the switch should mark packets' ECN
# bits as "Congestion Experienced".
DCTCP_THRESH = 40

# commands for building / running vhosts
DOCKER_IMAGE = 'etalon'
DOCKER_CLEAN = 'sudo docker ps -q | xargs sudo docker stop -t 0 ' \
               '2> /dev/null; ' \
               'sudo docker ps -aq | xargs sudo docker rm 2> /dev/null'
DOCKER_BUILD = 'sudo docker build -t %s -f ' \
'/etalon/vhost/etalon.dockerfile /etalon/vhost/' % DOCKER_IMAGE
DOCKER_LOCAL_IMAGE_PATH = '/etalon/vhost/etalon.img'
DOCKER_SAVE = 'sudo docker save -o {img} etalon && sudo chown ' \
              '`whoami` {img}'.format(img=DOCKER_LOCAL_IMAGE_PATH)
DOCKER_REMOTE_IMAGE_PATH = '/etalon/vhost/etalon.img'
DOCKER_LOAD = 'sudo docker load -i %s' % DOCKER_REMOTE_IMAGE_PATH
DOCKER_RUN = 'sudo docker run -d -h h{id}.{FQDN} -v ' \
             '{hosts_file}:/etc/hosts:ro ' \
             '--cpuset-cpus={cpu_set} -c {cpu_limit} --name=h{id} '\
             '--sysctl net.core.somaxconn=2048 --sysctl net.ipv4.tcp_max_syn_backlog=2048 '\
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
           '10.{net}.{rack}.{id}/16'
TC = 'sudo pipework tc h{id} qdisc add dev {int_if} root netem rate {rate}gbit'
SWITCH_PING = 'ping switch -c1'
GET_SWITCH_MAC = "arp | grep switch | tr -s ' ' | cut -d' ' -f3"
ARP_POISON = 'arp -s h{id} {switch_mac}'
SET_CC = 'sudo sysctl -w net.ipv4.tcp_congestion_control={cc}'
SCP = 'scp -r -o StrictHostKeyChecking=no root@%s:%s %s'
SCP_TO = 'scp -r -o StrictHostKeyChecking=no %s %s:%s'

# temporary files
DID_BUILD_FN = '/tmp/docker_built'
HOSTS_FILE = '/tmp/hosts'
REMOVE_HOSTS_FILE = 'sudo rm -rf %s' % (HOSTS_FILE)
SLAVES_FILE = '/etalon/vhost/config/hadoop_config/slaves'

# hdfs benchmark
DFSIOE = '/root/HiBench/bin/workloads/micro/dfsioe/hadoop/run_write.sh'

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
    'flowgrindd': '"pipework --wait -i eth1 && pipework --wait -i eth2 && '
                  'LD_PRELOAD=libVT.so taskset -c {cpu} '
                  'flowgrindd -d -c {cpu}"',

    'flowgrindd_adu': '"pipework --wait -i eth1 && pipework --wait -i eth2 && '
                      'LD_PRELOAD=libVT.so:libADU.so taskset -c {cpu} '
                      'flowgrindd -d -c {cpu}"',

    'HDFS': '"service ssh start && '
            'pipework --wait -i eth1 && pipework --wait -i eth2 && sleep infinity"',

    'HDFS_adu': '"echo export LD_PRELOAD=libADU.so >> /root/.bashrc && '
                'export LD_PRELOAD=libADU.so && '
                'echo LD_PRELOAD=libADU.so >> /root/.ssh/environment && '
                'service ssh start && '
                'pipework --wait -i eth1 && pipework --wait -i eth2 && '
                'sleep infinity"',

    'HDFS_nn': '"service ssh start && '
               'pipework --wait -i eth1 && pipework --wait -i eth2 && '
               '/usr/local/hadoop/bin/hdfs namenode -format -force && '
               '/usr/local/hadoop/sbin/start-dfs.sh && '
               '/usr/local/hadoop/sbin/start-yarn.sh && '
               '/usr/local/hadoop/sbin/mr-jobhistory-daemon.sh start '
               'historyserver && '
               'sleep infinity"',

    'HDFS_nn_adu': '"echo export LD_PRELOAD=libADU.so >> /root/.bashrc && '
                   'export LD_PRELOAD=libADU.so && '
                   'echo LD_PRELOAD=libADU.so >> /root/.ssh/environment && '
                   'service ssh start && '
                   'pipework --wait -i eth1 && pipework --wait -i eth2 && '
                   '/usr/local/hadoop/bin/hdfs namenode -format -force && '
                   '/usr/local/hadoop/sbin/start-dfs.sh && '
                   '/usr/local/hadoop/sbin/start-yarn.sh && '
                   '/usr/local/hadoop/sbin/mr-jobhistory-daemon.sh start '
                   'historyserver && '
                   'sleep infinity"',

    'reHDFS': '"service ssh start && '
              'sed -i s/org.apache.hadoop.hdfs.server.blockmanagement.'
              'BlockPlacementPolicyDefault/'
              'org.apache.hadoop.hdfs.server.blockmanagement.'
              'BlockPlacementPolicyRDCN/ '
              '/usr/local/hadoop/etc/hadoop/hdfs-site.xml &&'
              'pipework --wait -i eth1 && pipework --wait -i eth2 && sleep infinity"',

    'reHDFS_adu': '"echo export LD_PRELOAD=libADU.so >> /root/.bashrc && '
                  'export LD_PRELOAD=libADU.so && '
                  'echo LD_PRELOAD=libADU.so >> /root/.ssh/environment && '
                  'service ssh start && '
                  'sed -i s/org.apache.hadoop.hdfs.server.blockmanagement.'
                  'BlockPlacementPolicyDefault/'
                  'org.apache.hadoop.hdfs.server.blockmanagement.'
                  'BlockPlacementPolicyRDCN/ '
                  '/usr/local/hadoop/etc/hadoop/hdfs-site.xml &&'
                  'pipework --wait -i eth1 && pipework --wait -i eth2 && '
                  'sleep infinity"',

    'reHDFS_nn': '"service ssh start && '
                 'sed -i s/org.apache.hadoop.hdfs.server.blockmanagement.'
                 'BlockPlacementPolicyDefault/'
                 'org.apache.hadoop.hdfs.server.blockmanagement.'
                 'BlockPlacementPolicyRDCN/ '
                 '/usr/local/hadoop/etc/hadoop/hdfs-site.xml &&'
                 'pipework --wait -i eth1 && pipework --wait -i eth2 && '
                 '/usr/local/hadoop/bin/hdfs namenode -format -force && '
                 '/usr/local/hadoop/sbin/start-dfs.sh && '
                 '/usr/local/hadoop/sbin/start-yarn.sh && '
                 '/usr/local/hadoop/sbin/mr-jobhistory-daemon.sh start '
                 'historyserver && '
                 'sleep infinity"',

    'reHDFS_nn_adu': '"echo export LD_PRELOAD=libADU.so >> /root/.bashrc && '
                     'export LD_PRELOAD=libADU.so && '
                     'echo LD_PRELOAD=libADU.so >> /root/.ssh/environment && '
                     'service ssh start && '
                     'sed -i s/org.apache.hadoop.hdfs.server.blockmanagement.'
                     'BlockPlacementPolicyDefault/'
                     'org.apache.hadoop.hdfs.server.blockmanagement.'
                     'BlockPlacementPolicyRDCN/ '
                     '/usr/local/hadoop/etc/hadoop/hdfs-site.xml &&'
                     'pipework --wait -i eth1 && pipework --wait -i eth2 && '
                     '/usr/local/hadoop/bin/hdfs namenode -format -force && '
                     '/usr/local/hadoop/sbin/start-dfs.sh && '
                     '/usr/local/hadoop/sbin/start-yarn.sh && '
                     '/usr/local/hadoop/sbin/mr-jobhistory-daemon.sh start '
                     'historyserver && '
                     'sleep infinity"',
}


# All available CC modules. Found by:
#     sudo sysctl net.ipv4.tcp_available_congestion_control
CCMS = ["reno", "cubic", "retcp", "dctcp", "bbr", "bic", "cdg", "highspeed",
        "htcp", "hybla", "illinois", "lp", "nv", "scalable", "vegas", "veno",
        "westwood", "yeah"]


# host1 --> apt105.apt.emulab.net
def handle_to_machine(h):
    machines = open(NODES_FILE, 'r').read().split('\n')[:-1]
    for machine in machines:
        handle, hostname = [m.strip() for m in machine.split('#')]
        if handle == h:
            return hostname
    return None


# host1 --> host1; h38 --> host3; 17 --> host1
def get_phost_from_host(h):
    if h in PHYSICAL_NODES:
        return h
    elif h[0] == 'h':
        return 'host%s' % h[1:2]
    else:
        return 'host%s' % h[0]


# host3 --> 3
def get_phost_id(phost):
    return int(phost.split('host')[-1])


# 3 --> host3
def get_phost_from_id(id):
    return 'host%d' % (id)


# (3, 7) --> h37
def get_host_from_rack_and_id(r, id):
    return 'h%d%d' % (r, id)


# (3, 7) --> h37.etalon.local
def get_hostname_from_rack_and_id(r, id):
    return 'h%d%d.%s' % (r, id, FQDN)


# host3 --> (100, 3); h37 --> (3, 7); h315 --> (3, 15)
def get_rack_and_id_from_host(h):
    if 'host' in h:
        return (PHOST_IP, get_phost_id(h))
    else:
        return int(h[1]), int(h[2:])


# (host3, CONTROL_NET) --> 10.2.100.3; (h37, DATA_NET) --> 10.1.3.7
def get_ip_from_host(h, net):
    r, s = get_rack_and_id_from_host(h)
    return '10.%s.%s.%s' % (net, r, s)


# host3 --> 10.1.100.3; h37 --> 10.1.3.7
def get_data_ip_from_host(h):
    return get_ip_from_host(h, DATA_NET)


# host3 --> 10.2.100.3; h37 --> 10.2.3.7
def get_control_ip_from_host(h):
    return get_ip_from_host(h, CONTROL_NET)


# generate /etc/hosts file
# includes preset entries for local host and switch.
# other entries are generated based on the number of racks and hosts per rack.
# vhost h11 is also designated the namenode (nn) for hdfs.
def gen_hosts_file(fn):
    fp = open(fn, 'w')
    fp.write('127.0.0.1\tlocalhost\n')
    fp.write('%s\tswitch\n' % (SWITCH_DATA_IP))
    fp.write('10.{net}.{rack}.{id}\tnn.{fqdn}\th{rack}{id}.{fqdn}\n'.format(
        net=DATA_NET, rack=1, id=1, fqdn=FQDN))
    for r in xrange(1, NUM_RACKS+1):
        for id in xrange(1, HOSTS_PER_RACK+1):
            if r == 1 and id == 1:
                continue
            fp.write('10.{net}.{rack}.{id}\th{rack}{id}.{fqdn}\n'.format(
                     net=DATA_NET, rack=r, id=id, fqdn=FQDN))
    fp.close()


# generate a slaves file for hdfs based on the number of racks
# and the number of hosts per rack.
def gen_slaves_file(fn):
    fp = open(fn, 'w')
    num_hosts = IMAGE_NUM_HOSTS['HDFS']
    for r in xrange(1, NUM_RACKS+1):
        for id in xrange(1, num_hosts+1):
            fp.write('h{r}{id}.{fqdn}\n'.format(r=r, id=id, fqdn=FQDN))
