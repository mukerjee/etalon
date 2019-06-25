#!/usr/bin/env python

import datetime
import socket
import time
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

    def log(self, msg):
        with open("/tmp/rpycd.log", "a+") as erf:
            erf.write("{}: {}".format(
                datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"), msg))

    # drops all connections that aren't from the switch
    def on_connect(self, conn):
        if conn._config['endpoints'][1][0] != SWITCH_CONTROL_IP:
            raise AssertionError("rpyc connection not from switch")

    def call(self, cmd, check_rc=True):
        try:
            output = check_output(cmd, shell=True)
            self.log("cmd: {} , output:\n{}".format(cmd, output))
            return output
        except CalledProcessError as e:
            if check_rc:
                self.log("cmd: {} , output: Error\nreturncode: {}\noutput:\n{}".format(
                    cmd, e.returncode, e.output))
                raise e

    # run on a container
    def exposed_run_host(self, my_id, my_cmd):
        return self.call(DOCKER_EXEC.format(id=my_id, cmd=my_cmd))

    # Run on the physical host but in a container's namespace. If "timeout_s" is greater than 0,
    # then attempt "my_cmd" every "interval_s" seconds until it suceeds, or until "timeout_s"
    # seconds have elapsed.
    def exposed_ns_run(self, my_id, my_cmd, timeout_s=0, interval_s=0):
        assert timeout_s >= 0, "\"timeout_s\" must >= 0, but is: {}".format(timeout_s)
        assert interval_s >= 0, "\"interval_s\" must >= 0, but is: {}".format(interval_s)

        my_pid = self.call(DOCKER_GET_PID.format(id=my_id)).split()[0].strip()
        full_cmd = NS_RUN.format(pid=my_pid, cmd=my_cmd)
        start_s = time.time() * 1000
        current_s = start_s
        end_s = start_s + timeout_s
        once = False
        count = 0
        while (not once) or (current_s - start_s < timeout_s):
            if not once:
                once = True
            try:
                return self.call(full_cmd)
            except CalledProcessError:
                self.log("Will try \"{}\" again".format(cmd))
            count += 1
            if current_s + interval_s > end_s:
                # If there is at least one interval remaining, then sleep for interval_s seconds.
                time.sleep(interval_s)
            current_s = time.time() * 1000
        raise RuntimeError(
            "Command \"{}\" did not complete successfully after {} attempts in {} seconds!".format(
                cmd, count))

    # run on a physical host
    def exposed_run(self, cmd):
        return self.call(cmd)


if __name__ == '__main__':
    from rpyc.utils.server import ThreadedServer
    t = ThreadedServer(EtalonService, port=RPYC_PORT, protocol_config={"allow_all_attrs": True})
    t.start()
