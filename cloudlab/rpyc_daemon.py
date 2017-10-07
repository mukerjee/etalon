#!/usr/bin/env python

import socket
import threading
import rpyc
from subprocess import call

RPYC_PORT = 18861

NUM_HOSTS = 8
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
CPU_LIMIT = (CPU_COUNT-1) * 100 / TDF  # 75

IMAGES = {
    'flowgrindd': 'mukerjee/sdrt-flowgrindd',
    'iperf': 'mukerjee/sdrt-iperf',
    'iperf3': 'mukerjee/sdrt-iperf3',
    'hadoop': 'mukerjee/sdrt-hadoop',
}


DOCKER_CLEAN = 'docker ps -q | xargs docker stop -t 0 2> /dev/null; ' \
               'docker ps -aq | xargs docker rm 2> /dev/null'
DOCKER_PULL = 'docker pull {image}'
DOCKER_RUN = 'docker run -d -h h{id} --cpuset-cpus={cpu_set} ' \
             '-c {cpu_limit} --name=h{id} {image}'
PIPEWORK = 'sudo pipework {ext_if} -i {int_if} h{id} ' \
           '10.10.{net}.{id}/24; sudo pipework tc h{id} qdisc add dev ' \
           '{int_if} root fq maxrate {rate}gbit'


class SDRTService(rpyc.Service):
    def on_connect(self):
        self.pulled = False

    def on_disconnection(self):
        pass

    def call(self, cmd):
        if not self.pulled:
            self.pulled = True
            self.update_images()
        print cmd
        call(cmd, shell=True)

    def clean(self):
        self.call(DOCKER_CLEAN)

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
        self.call(DOCKER_RUN.format(image=IMAGES[image],
                                    id=my_id, cpu_set=CPU_SET,
                                    cpu_limit=CPU_LIMIT))
        self.call(PIPEWORK.format(ext_if=DATA_EXT_IF, int_if=DATA_INT_IF,
                                  net=DATA_NET, id=my_id, rate=DATA_RATE))
        self.call(PIPEWORK.format(ext_if=CONTROL_EXT_IF, int_if=CONTROL_INT_IF,
                                  net=CONTROL_NET, id=my_id,
                                  rate=CONTROL_RATE))

    def launch_rack(self, image):
        self.clean()
        ts = []
        for i in xrange(1, NUM_HOSTS+1):
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


if __name__ == '__main__':
    from rpyc.utils.server import ThreadedServer
    t = ThreadedServer(SDRTService, port=RPYC_PORT)
    t.start()
