#!/usr/bin/env python

import datetime
import subprocess
import sys
import time

import rpyc
from rpyc.utils import server

sys.path.insert(0, '/etalon/etc')
import python_config


class EtalonService(rpyc.Service):

    def log(self, msg):
        with open("/tmp/rpycd.log", "a+") as erf:
            erf.write("{}: {}\n".format(
                datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"), msg))

    def on_connect(self, conn):
        """ Drops all connections that aren't from the switch. """
        assert (conn._config['endpoints'][1][0] == \
                python_config.SWITCH_CONTROL_IP), \
                "rpyc connection not from switch"

    def run(self, cmd, check_rc=True):
        try:
            self.log("Running cmd: {}".format(cmd))
            output = subprocess.check_output(cmd, shell=True)
            # For prettiness, only print a newline if there was some output.
            self.log("Finished cmd: {} , output:{}".format(
                cmd, "\n{}".format(output.strip()) if output != "" else ""))
            return output
        except subprocess.CalledProcessError as exp:
            if check_rc:
                self.log(
                    ("cmd: {} , output: Error\nreturncode: "
                     "{}\noutput:\n{}").format(cmd, exp.returncode, exp.output))
                raise exp

    def exposed_run_host(self, my_id, my_cmd):
        """ Run on a container. """
        return self.run(python_config.DOCKER_EXEC.format(id=my_id, cmd=my_cmd))

    def exposed_ns_run(self, my_id, my_cmd, timeout_s=0, interval_s=0):
        """
        Run on the physical host but in a container's namespace. If "timeout_s"
        is greater than 0, then attempt "my_cmd" every "interval_s" seconds
        until it suceeds, or until "timeout_s" seconds have elapsed.
        """
        assert timeout_s >= 0, \
            "\"timeout_s\" must be >= 0, but is: {}".format(timeout_s)
        assert interval_s >= 0, \
            "\"interval_s\" must be >= 0, but is: {}".format(interval_s)

        my_pid = self.run(cmd=python_config.DOCKER_GET_PID.format(
            id=my_id)).split()[0].strip()
        full_cmd = python_config.NS_RUN.format(pid=my_pid, cmd=my_cmd)
        start_s = time.time()
        current_s = start_s
        end_s = start_s + timeout_s
        once = False
        count = 0
        while (not once) or (current_s - start_s < timeout_s):
            once = True
            try:
                return self.run(full_cmd)
            except subprocess.CalledProcessError:
                count += 1
                self.log(("Will try command \"{}\" on host \"{}\" again "
                          "(attempt #{}) in {} second(s).").format(
                              my_cmd, my_id, count + 1, interval_s))
            if current_s + interval_s > end_s:
                # If there is at least one interval remaining, then sleep for
                # interval_s seconds.
                time.sleep(interval_s)
            current_s = time.time()
        msg = ("Command \"{}\" on host \"{}\" did not complete successfully "
               "after {} attempt(s) in {} seconds!").format(
                   my_cmd, my_id, count, timeout_s)
        self.log(msg)
        raise RuntimeError(msg)

    def exposed_run(self, cmd):
        """ Run on a physical host. """
        return self.run(cmd)


if __name__ == '__main__':
    server.ThreadedServer(EtalonService, port=python_config.RPYC_PORT,
                          protocol_config={"allow_all_attrs": True}).start()
