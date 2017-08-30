import os
import subprocess
import threading
import Queue
import time
import numpy

class MonitorObject(object):
    """Process's info from ps command"""
    def __init__(self, pid, pcpu, cputime, etimes, state, nlwp):
        super(MonitorObject, self).__init__()
        self.pid = float(pid)
        self.pcpu = float(pcpu)
        hhmmss = [ int(x) for x in cputime.split(":") ]
        self.cputimes = hhmmss[0] * 3600 + hhmmss[1] * 60 + hhmmss[2]
        self.etimes = int(etimes)
        self.state = state
        self.nlwp = int(nlwp)
    
    def __str__(self):
        return "%s" % self.pid

    def __eq__(self, other):
        if isinstance(other, self.__class__):
            return other.pid == self.pid
        else:
            return False

class MininetMonitor(threading.Thread):
    """use ps to get cpu usage"""
    def __init__(self, result_q, sample_interval = 1, pid_list = []):
        super(MininetMonitor, self).__init__()
        self.pid_list = []
        if len(pid_list) > 0:
            self.pid_list.extend(pid_list)

        self.result_q = result_q
        self.sample_interval = sample_interval # unit in second
        self.stop_request = threading.Event()

        self.last_processed = [] # process history
        self.cpu_usage = None
        self.cpu_history = [] # cpu usage history
        self.queue_history = [] # number threads in queue
        self.thread_history = []

        # fixed processes's PIDs can be inited here
        ovs_vswitchd = self.pexec_pgrep("ovs-vswitchd")
        ovsdb_server = self.pexec_pgrep("ovsdb-server")
        ovs_vsctl = self.pexec_pgrep("ovs-vsctl")
        self.addProcessID(ovs_vswitchd)
        self.addProcessID(ovsdb_server)
        self.addProcessID(ovs_vsctl)

        # self.debug_file = open('debug_monitor.log', 'a+')
        # self.debug_file.write("a new monitor thread is created\n")
        self.record_file = open('record_load.log', 'a+')
        self.record_file.write("******************\nNew Expriment Starts\n******************\n")

    def run(self):
        while not self.stop_request.isSet():
            self.samplingCPUUsage()
            time.sleep(self.sample_interval)
        # self.debug_file.close()
        # self.record_file.close()
        # print "*** Monitor stopped\n"

    def join(self, timeout = None):
        if self.cpu_usage != None:
            # self.result_q.put(self.cpu_usage)
            self.record_file.write( "*** Frm Mt: %%CPU = %f\n" % self.cpu_usage )
        else:
            # self.result_q.put(None)
            pass

        if len(self.cpu_history) > 0:
            avg_cpu_usage = numpy.mean(self.cpu_history) # sum(self.cpu_history) / len(self.cpu_history)
            self.record_file.write( "*** Mt Sum: Avg %%CPU = %f\n" % avg_cpu_usage )
            self.result_q.put(avg_cpu_usage)
        else:
            self.result_q.put(None)
        if len(self.queue_history) > 0:
            avg_queue_len = numpy.sum(self.queue_history)
            self.record_file.write( "*** Mt Sum: Tot Q Len = %d\n" % avg_queue_len )
        if len(self.thread_history) > 0:
            avg_threads = numpy.mean(self.thread_history)
            self.record_file.write( "*** Mt Sum: Avg Threads = %f\n" % avg_threads )


        # not necessary, but to make sure...
        self.cpu_history = []
        self.queue_history = []
        self.thread_history = []
        self.stop_request.set()
        super(MininetMonitor, self).join(timeout)

    def addProcessID(self, pid):
        if pid != None:
            self.pid_list.append(str(pid))

    def addProcess(self, cmd):
        """ add to process id list, given its cmd name """
        pid = self.pexec_pgrep(cmd)
        if pid != None:
            self.addProcessID(pid)

    def pexec_pgrep(self, process_name):
        cmd = "pgrep %s" % process_name
        p = subprocess.Popen(cmd, stdout = subprocess.PIPE, shell = True)
        (pid, err) = p.communicate()
        p_status = p.wait()
        if p_status == 0: # else return None
            return pid.strip()

    def pexec_ps(self, pid_list, ppid_list):

        # cmd = "ps --pid %s --ppid %s -o %%cpu --no-headers" % (pid_list, ppid_list)
        cmd = "ps --pid %s --ppid %s -o %%cpu,cputime,etimes,pid,stat,nlwp --no-headers" % (pid_list, ppid_list)
        p = subprocess.Popen(cmd, stdout = subprocess.PIPE, shell = True)
        (output, err) = p.communicate()
        p_status = p.wait()
        if p_status == 0:
            return output

    def samplingCPUUsage(self):
        pid_list_str = ""
        for pid in self.pid_list:
            pid_list_str += str(pid)
            if pid != self.pid_list[-1]:
                pid_list_str += ','
        ppid_list_str = pid_list_str
        # self.debug_file.write("# of items in pid list in ps = %d\n" % len( pid_list_str ))
        output = self.pexec_ps(pid_list_str, ppid_list_str)

        tmp = output.split()
        processes = []
        num_fields = 6
        for i in xrange(0, len(tmp), num_fields):
            pcpu = tmp[i]
            cputime = tmp[ i + 1 ]
            etimes = tmp[ i + 2 ]
            pid = tmp[ i + 3 ]
            state = tmp[ i + 4 ]
            nlwp = tmp[ i + 5 ]
            process = MonitorObject(pid, pcpu, cputime, etimes, state, nlwp)
            processes.append(process)

        cpu_usage = 0
        running = 0
        lwthreads = 0
        for new in processes:
            if 'R' in new.state:
                running += 1
                if new.nlwp != 0:
                    lwthreads += new.nlwp

            for old in self.last_processed:
                if new == old:
                    tmp = (new.cputimes - old.cputimes) / (new.etimes - old.etimes)
                    cpu_usage += tmp
                    continue
            cpu_usage += new.pcpu
            self.last_processed.append(new)


        self.cpu_usage = cpu_usage
        self.cpu_history.append(cpu_usage)
        self.queue_history.append(running)
        self.thread_history.append(lwthreads)
        self.record_file.write( "Mt Sampling: QLen %d\t #Threads %d\n" % (running, lwthreads) )
        # print "Mt Rcd: %%CPU = %f\t%d in running Q" % (self.cpu_usage, running)

def testMininetMonitor():
    result_q = Queue.Queue()

    mm = MininetMonitor(result_q, 1, [5917, 3875])
    # mm.addProcessID(1) # init's pid
    mm.addProcess("mn")
    mm.addProcessID(3886)
    print mm.pid_list
    mm.start()
    time.sleep(5)
    mm.join()
    print result_q.get()


if __name__ == '__main__':
    testMininetMonitor()


