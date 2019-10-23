#!/usr/bin/env python

import datetime
import multiprocessing
import subprocess
import sys
# For python_config.
sys.path.insert(0, '/etalon/etc')
import time

import rpyc
from rpyc.utils import server

import python_config


class EtalonService(rpyc.Service):

    def log(self, msg):
        with open("/tmp/rpycd.log", "a+") as lgf:
            lgf.write("{}: {}\n".format(
                datetime.datetime.now().strftime("%Y-%m-%d %H:%M:%S"), msg))

    def on_connect(self, conn):
        """ Drops all connections that aren't from the switch. """
        assert (conn._config['endpoints'][1][0] == \
                python_config.SWITCH_CONTROL_IP), \
                "rpyc connection not from switch"

    def exposed_run(self, cmd):
        """
        Run a command on the physical host, but do not wait for the command to
        complete. Note: Any errors or output from the command are hidden.
        """
        self.log("Launching cmd: {}".format(cmd))
        multiprocessing.Process(target=lambda : self.exposed_run_fully(cmd)).start()
        self.log("Launched cmd: {}".format(cmd))

    def exposed_run_fully(self, cmd):
        """ Run a command to completion on the physical host. """
        self.log("Running cmd: {}".format(cmd))
        try:
            out = subprocess.check_output(cmd, shell=True)
            out_msg = "\n{}".format(out.strip()) if out != "" else ""
            return out
        except subprocess.CalledProcessError as exp:
            # TODO: Log exp.child_traceback.
            out_msg = "Error , returncode: {} , exception output:\n{}".format(
                exp.returncode, exp.output)
            raise exp
        finally:
            self.log("Finished cmd: {} , output: {}".format(cmd, out_msg))

    def exposed_run_fully_host(self, hid, cmd):
        """ Run a command to completion in a host container. """
        return self.exposed_run_fully(
            cmd=python_config.DOCKER_EXEC.format(id=hid, cmd=cmd))

    def exposed_run_fully_host_ns(self, hid, cmd, timeout_s=0, interval_s=0):
        """
        Run a command to completion on the physical host but in a container's
        namespace. If "timeout_s" is greater than 0, then attempt the command
        every "interval_s" seconds until it suceeds or "timeout_s" seconds have
        elapsed.
        """
        assert timeout_s >= 0, \
            "\"timeout_s\" must be >= 0, but is: {}".format(timeout_s)
        assert interval_s >= 0, \
            "\"interval_s\" must be >= 0, but is: {}".format(interval_s)

        once = False
        count = 0
        start_s = time.time()
        current_s = start_s
        end_s = start_s + timeout_s
        while (not once) or (current_s - start_s < timeout_s):
            once = True
            try:
                # First, get the Docker container's PID, then run the command in
                # the container's namespace.
                return self.exposed_run_fully(python_config.NS_RUN.format(
                    pid=self.exposed_run_fully(
                        cmd=python_config.DOCKER_GET_PID.format(
                            id=hid)).split()[0].strip(),
                    cmd=cmd))
            except subprocess.CalledProcessError:
                count += 1
                self.log(("Will try command \"{}\" on host \"{}\" again "
                          "(attempt #{}) in {} second(s).").format(
                              cmd, hid, count + 1, interval_s))
            if current_s + interval_s > end_s:
                # If there is at least one interval remaining, then sleep for
                # "interval_s" seconds.
                time.sleep(interval_s)
            current_s = time.time()

        # If we are here, then the command never completed successfully.
        msg = ("Command \"{}\" on host \"{}\" did not complete successfully "
               "after {} attempt(s) in {} seconds!").format(
                   cmd, hid, count, timeout_s)
        self.log(msg)
        raise RuntimeError(msg)


if __name__ == '__main__':
    server.ThreadedServer(EtalonService, port=python_config.RPYC_PORT,
                          protocol_config={"allow_all_attrs": True}).start()
