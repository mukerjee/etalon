#!/usr/bin/env python

import socket
import threading
import rpyc
from subprocess import check_output, CalledProcessError, Popen

RPYC_PORT = 18861

NUM_RACKS = 8
HOSTS_PER_RACK = 16
TDF = 20.0
SELF_ID = int(socket.gethostname().split('.')[0][-1])

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

IMAGES = {
    'flowgrindd': 'mukerjee/sdrt-flowgrindd',
    'iperf': 'mukerjee/sdrt-iperf',
    'iperf3': 'mukerjee/sdrt-iperf3',
    'hadoop': 'mukerjee/sdrt-hadoop',
}


DOCKER_CLEAN = 'sudo docker ps -q | xargs sudo docker stop -t 0 ' \
               '2> /dev/null; ' \
               'sudo docker ps -aq | xargs sudo docker rm 2> /dev/null'
DOCKER_PULL = 'sudo docker pull {image}'
DOCKER_RUN = 'sudo docker run -d -h h{id} --cpuset-cpus={cpu_set} ' \
             '-c {cpu_limit} --name=h{id} {image} {cmd}'
DOCKER_GET_PID = "sudo docker inspect --format '{{{{.State.Pid}}}}' h{id}"
PIPEWORK = 'sudo pipework {ext_if} -i {int_if} h{rack}{id} ' \
           '10.{net}.{rack}.{id}/16; '
TC = 'sudo pipework tc h{id} qdisc add dev {int_if} root netem rate {rate}gbit'
NS_RUN = 'sudo nsenter -t {pid} -n {cmd}'
SWITCH_PING = 'ping switch -c1'
GET_SWITCH_MAC = "arp | grep switch | tr -s ' ' | cut -d' ' -f3"
ARP_POISON = 'arp -s h{id} {switch_mac}'

KILL_PING = 'sudo killall -s SIGINT ping 2> /dev/null'
PING = 'sudo LD_PRELOAD=libVT.so ping -i {interval} -D -U {dest}'

KILL_SOCKPERF = 'sudo killall -s SIGINT sockperf 2> /dev/null'
SOCKPERF = "sudo LD_PRELOAD=libVT.so sockperf pp -i " \
           "`getent hosts {dest} | awk '{{print $1}}'` -t2"
SOCKPERF_DAEMON = 'LD_PRELOAD=libVT.so sockperf sr --daemonize'

RTO_MIN='ip route change 10.{net}.0.0/16 dev {int_if} proto kernel ' \
    'scope link src 10.{net}.{rack}.{id} rto_min 1ms'

class SDRTService(rpyc.Service):
    def on_connect(self):
        self.pulled = False

    def on_disconnection(self):
        pass

    def call_background(self, cmd):
        return Popen(cmd, shell=True)

    def call(self, cmd, check_rc=True):
        if not self.pulled:
            self.pulled = True
            self.update_images()
        print cmd
        try:
            return check_output(cmd, shell=True)
        except CalledProcessError as e:
            if check_rc:
                raise e

    def clean(self):
        self.call(DOCKER_CLEAN, check_rc=False)

    def update_image(self, img):
        self.call(DOCKER_PULL.format(image=img))

    def update_images(self):
        ts = []
        for img in IMAGES.values():
            ts.append(threading.Thread(target=self.update_image,
                                       args=(img,)))
            ts[-1].start()
        map(lambda t: t.join(), ts)

    def launch(self, image, host_id):
        my_id = '%d%d' % (SELF_ID, host_id)
        cpus = str((host_id % (CPU_COUNT-1)) + 1) if image == 'flowgrindd' else CPU_SET
        my_cmd = '"pipework --wait && pipework --wait -i eth2 && ' \
                 '/root/on_run.sh && LD_PRELOAD=libVT.so taskset -c {cpu} ' \
                 'flowgrindd -d -c {cpu}"'.format(cpu=cpus) \
                 if image == 'flowgrindd' else ''
        my_cmd = '/bin/sh -c ' + my_cmd
        self.call(DOCKER_RUN.format(image=IMAGES[image],
                                    id=my_id, cpu_set=cpus,
                                    cpu_limit=CPU_LIMIT, cmd=my_cmd))
        self.call(PIPEWORK.format(ext_if=DATA_EXT_IF, int_if=DATA_INT_IF,
                                  net=DATA_NET, rack=SELF_ID, id=host_id))
        self.call(TC.format(int_if=DATA_INT_IF, id=my_id, rate=DATA_RATE))
        self.call(PIPEWORK.format(ext_if=CONTROL_EXT_IF, int_if=CONTROL_INT_IF,
                                  net=CONTROL_NET, rack=SELF_ID, id=host_id))
        self.call(TC.format(int_if=CONTROL_INT_IF, id=my_id,
                            rate=CONTROL_RATE))
        my_pid = self.call(DOCKER_GET_PID.format(id=my_id)).split()[0].strip()
        self.call(NS_RUN.format(pid=my_pid, cmd=RTO_MIN.format(net=DATA_NET,
                                                               int_if=DATA_INT_IF,
                                                               rack=SELF_ID,
                                                               id=host_id)))
        self.call(NS_RUN.format(pid=my_pid, cmd=SWITCH_PING))
        smac = self.call(NS_RUN.format(pid=my_pid, cmd=GET_SWITCH_MAC))

        my_rack = int(my_id[0])
        for i in xrange(1, NUM_RACKS+1):
            if i == my_rack:
                continue
            for j in xrange(1, HOSTS_PER_RACK+1):
                dst_id = '%d%d' % (i, j)
                self.call(NS_RUN.format(pid=my_pid,
                                        cmd=ARP_POISON.format(
                                            id=dst_id, switch_mac=smac)))

    def launch_rack(self, image):
        self.clean()
        ts = []
        for i in xrange(1, HOSTS_PER_RACK+1):
            ts.append(threading.Thread(target=self.launch,
                                       args=(image, i)))
            ts[-1].start()
        map(lambda t: t.join(), ts)

    def exposed_flowgrindd(self):
        self.launch_rack('flowgrindd')

    def exposed_iperf_server(self):
        self.launch_rack('iperf')

    def exposed_iperf3_server(self):
        self.launch_rack('iperf3')

    def exposed_hadoop(self):
        self.launch_rack('hadoop')

    def exposed_kill_all_ping(self):
        self.call(KILL_PING, check_rc=False)

    def exposed_ping(self, dst):
        intv = '%f' % (0.0001 / TDF)
        return self.call(PING.format(interval=intv, dest=dst))

    def exposed_kill_all_sockperf(self):
        self.call(KILL_SOCKPERF, check_rc=False)

    def exposed_sockperf(self, dst):
        return self.call(SOCKPERF.format(dest=dst))

    def exposed_launch_sockperf_daemon(self):
        return self.call_background(SOCKPERF_DAEMON)

if __name__ == '__main__':
    from rpyc.utils.server import ThreadedServer
    t = ThreadedServer(SDRTService, port=RPYC_PORT)
    t.start()
