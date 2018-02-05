#!/usr/bin/env python

import socket
import threading
import rpyc
from subprocess import check_output, CalledProcessError, Popen

SELF_ID = int(socket.gethostname().split('.')[0][-1])
SET_CC = 'sudo sysctl -w net.ipv4.tcp_congestion_control={cc}'

class SDRTService(rpyc.Service):
    def call_background(self, cmd):
        return Popen(cmd, shell=True)

    def call(self, cmd, check_rc=True):
        print cmd
        try:
            return check_output(cmd, shell=True)
        except CalledProcessError as e:
            if check_rc:
                raise e

    def clean(self):
        self.call(DOCKER_CLEAN, check_rc=False)

    def update_image(self, img):
        self.call(DOCKER_BUILD.format(image=img))

    def launch(self, image, host_id):
        my_id = '%d%d' % (SELF_ID, host_id)
        cpus = str((host_id % (CPU_COUNT-1)) + 1) \
            if 'flowgrindd' in image else CPU_SET
        my_cmd = ''
        if image == 'flowgrindd':
            my_cmd = '"pipework --wait && pipework --wait -i eth2 && ' \
                     'LD_PRELOAD=libVT.so taskset -c {cpu} ' \
                     'flowgrindd -d -c {cpu}"'.format(cpu=cpus)
        if image == 'flowgrindd_adu':
            my_cmd = '"pipework --wait && pipework --wait -i eth2 && ' \
                     'LD_PRELOAD=libVT.so:libADU.so taskset -c {cpu} ' \
                     'flowgrindd -d -c {cpu}"'.format(cpu=cpus)
            image = 'flowgrindd'
        if 'hadoop' in image:
            if 'adu'  in image:
                my_cmd = '"echo export LD_PRELOAD=libADU.so >> /root/.bashrc && export LD_PRELOAD=libADU.so && '\
                         'echo PermitUserEnvironment yes >> /etc/ssh/sshd_config && '\
                         'echo LD_PRELOAD=libADU.so > /root/.ssh/environment && '\
                         'service ssh start && ' \
                         'pipework --wait && pipework --wait -i eth2 && sleep infinity"'
            else:
                my_cmd = '"service ssh start && ' \
                         'pipework --wait && pipework --wait -i eth2 && sleep infinity"'
        if 'hadoop' in image and my_id == '11':
            if 'adu' in image:
                my_cmd = '"echo export LD_PRELOAD=libADU.so >> /root/.bashrc && export LD_PRELOAD=libADU.so && ' \
                         'echo PermitUserEnvironment yes >> /etc/ssh/sshd_config && '\
                         'echo LD_PRELOAD=libADU.so > /root/.ssh/environment && '\
                         'service ssh start && ' \
                         'pipework --wait && pipework --wait -i eth2 && ' \
                         '/usr/local/hadoop/bin/hdfs namenode -format -force && '\
                         '/tmp/config/start_hadoop.sh && sleep infinity"'
            else:
                my_cmd = '"service ssh start && ' \
                         'pipework --wait && pipework --wait -i eth2 && ' \
                         '/usr/local/hadoop/bin/hdfs namenode -format -force && '\
                         '/tmp/config/start_hadoop.sh && sleep infinity"'
        my_cmd = '/bin/sh -c ' + my_cmd
        cpu_lim = 1500 if 'hadoop' in image else CPU_LIMIT
        self.call(DOCKER_RUN.format(image=IMAGES[image],
                                    id=my_id, cpu_set=cpus,
                                    cpu_limit=cpu_lim, cmd=my_cmd))
        self.call(PIPEWORK.format(ext_if=DATA_EXT_IF, int_if=DATA_INT_IF,
                                  net=DATA_NET, rack=SELF_ID, id=host_id))
        if 'hadoop' not in image:
            self.call(TC.format(int_if=DATA_INT_IF, id=my_id, rate=DATA_RATE))
        my_pid = self.call(DOCKER_GET_PID.format(id=my_id)).split()[0].strip()
        self.call(NS_RUN.format(pid=my_pid, cmd=SWITCH_PING))
        smac = self.call(NS_RUN.format(pid=my_pid, cmd=GET_SWITCH_MAC))

        my_rack = int(my_id[0])
        for i in xrange(1, NUM_RACKS+1):
            if i == my_rack:
                continue
            for j in xrange(1, HOSTS_PER_RACK+1):
                dst_id = '%d%d.sdrt.cs.cmu.edu' % (i, j)
                self.call(NS_RUN.format(pid=my_pid,
                                        cmd=ARP_POISON.format(
                                            id=dst_id, switch_mac=smac)))

        self.call(PIPEWORK.format(ext_if=CONTROL_EXT_IF, int_if=CONTROL_INT_IF,
                                  net=CONTROL_NET, rack=SELF_ID, id=host_id))
        if 'hadoop' not in image:
            self.call(TC.format(int_if=CONTROL_INT_IF, id=my_id,
                                rate=CONTROL_RATE))

    def launch_rack(self, image):
        self.clean()
        self.pulled = True
        self.update_image(IMAGES[image])
        ts = []
        num_hosts = 1 if 'hadoop' in image else HOSTS_PER_RACK
        for i in xrange(1, num_hosts+1):
            ts.append(threading.Thread(target=self.launch,
                                       args=(image, i)))
            ts[-1].start()
        map(lambda t: t.join(), ts)

    def exposed_run_host(self, my_cmd, host_id):
        my_id = '%d%d' % (SELF_ID, host_id)
        return self.call(DOCKER_EXEC.format(id=my_id, cmd=my_cmd), check_rc=False)

    def exposed_set_cc(self, new_cc):
        self.call(SET_CC.format(cc=new_cc))


if __name__ == '__main__':
    from rpyc.utils.server import ThreadedServer
    t = ThreadedServer(SDRTService, port=RPYC_PORT)
    t.start()
