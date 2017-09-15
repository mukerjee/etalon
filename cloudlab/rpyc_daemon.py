#!/usr/bin/env python

import rpyc
from subprocess import Popen, PIPE

rpyc.core.protocol.DEFAULT_CONFIG['allow_pickle'] = True

RPYC_PORT = 18861

iperf3_client = 'iperf3 -i0.1 -t%s -c %s'
ping = 'sudo ping -i0.05 -w%s %s'

class MyService(rpyc.Service):
    def on_connect(self):
        pass

    def on_disconnection(self):
        pass

    def exposed_iperf3(self, host, time=10):        
        cmd = iperf3_client % (time, host)
        p = Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE)
        sout, serr = p.communicate()
        return p.returncode, sout, serr

    def exposed_ping(self, host, time=10):        
        cmd = ping % (time, host)
        print cmd
        p = Popen(cmd, shell=True, stdout=PIPE, stderr=PIPE)
        sout, serr = p.communicate()
        return p.returncode, sout, serr

    def exposed_uname(self):
        call

if __name__ == '__main__':
    from rpyc.utils.server import ThreadedServer
    t = ThreadedServer(MyService, port=RPYC_PORT, 
                       protocol_config=rpyc.core.protocol.DEFAULT_CONFIG)
    t.start()
