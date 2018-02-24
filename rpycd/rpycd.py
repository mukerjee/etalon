#!/usr/bin/env python

import socket
import rpyc

from subprocess import check_output, CalledProcessError
import sys
sys.path.insert(0, '/etalon/etc')
from python_config import RPYC_PORT, SWITCH_CONTROL_IP

DOCKER_EXEC = 'sudo docker exec -t h{id} {cmd}'
DOCKER_GET_PID = "sudo docker inspect --format '{{{{.State.Pid}}}}' h{id}"
NS_RUN = 'sudo nsenter -t {pid} -n {cmd}'
SELF_ID = int(socket.gethostname().split('.')[0][-1])


class EtalonService(rpyc.Service):
    # drops all connections that aren't from the switch
    def on_connect(self):
        if self._conn._config['endpoints'][1][0] != SWITCH_CONTROL_IP:
            raise AssertionError("rpyc connection not from switch")
        
    def call(self, cmd, check_rc=True):
        print cmd
        try:
            return check_output(cmd, shell=True)
        except CalledProcessError as e:
            if check_rc:
                raise e

    # run on a container
    def exposed_run_host(self, my_id, my_cmd):
        return self.call(DOCKER_EXEC.format(id=my_id, cmd=my_cmd))

    # run on the physical host but in a container's namespace
    def exposed_ns_run(self, my_id, my_cmd):
        my_pid = self.call(DOCKER_GET_PID.format(id=my_id)).split()[0].strip()
        return self.call(NS_RUN.format(pid=my_pid, cmd=my_cmd))

    # run on a physical host
    def exposed_run(self, my_cmd):
        return self.call(cmd=my_cmd)


if __name__ == '__main__':
    from rpyc.utils.server import ThreadedServer
    t = ThreadedServer(EtalonService, port=RPYC_PORT)
    t.start()
