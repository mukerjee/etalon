
import datetime
import glob
import os
from os import path
import socket
from subprocess import call, PIPE, STDOUT, Popen
import sys
# Directory containing this program.
PROGDIR = path.dirname(path.realpath(__file__))
# For python_config.
sys.path.insert(0, path.join(PROGDIR, "..", "..", "etc"))
import tarfile
import threading
import time

import numpy as np
import rpyc

import click_common
from python_config import NUM_RACKS, HOSTS_PER_RACK, TIMESTAMP, SCRIPT, \
    EXPERIMENTS, PHYSICAL_NODES, RPYC_CONNECTIONS, RPYC_PORT, CIRCUIT_BW_Gbps, \
    PACKET_BW_Gbps, FQDN, DATA_NET, CONTROL_NET, DFSIOE, get_phost_from_host, \
    get_phost_id, IMAGE_CPU, CPU_COUNT, CPU_SET, DEFAULT_REQUEST_SIZE, \
    IMAGE_CMD, IMAGE_SETUP, DOCKER_IMAGE, PIPEWORK, DATA_EXT_IF, DATA_INT_IF, \
    IMAGE_SKIP_TC, TC, DATA_RATE_Gbps_TDF, SWITCH_PING, GET_SWITCH_MAC, \
    ARP_POISON, CONTROL_EXT_IF, CONTROL_INT_IF, CONTROL_RATE_Gbps_TDF, \
    DOCKER_CLEAN, IMAGE_NUM_HOSTS, DOCKER_BUILD, \
    SET_CC, get_host_from_rack_and_id, SCP, get_data_ip_from_host, \
    get_control_ip_from_host, FLOWGRIND_PORT, HDFS_PORT, DOCKER_SAVE, SCP_TO, \
    DOCKER_LOCAL_IMAGE_PATH, DOCKER_REMOTE_IMAGE_PATH, DOCKER_LOAD, \
    get_phost_from_id, DID_BUILD_FN, gen_hosts_file, HOSTS_FILE, \
    IMAGE_DOCKER_RUN, REMOVE_HOSTS_FILE, gen_slaves_file, SLAVES_FILE, \
    get_hostname_from_rack_and_id, get_rack_and_id_from_host, DEFAULT_CC, \
    FLOWGRIND_DEFAULT_DUR_S, FLOWGRIND_DEFAULT_SAMPLE_RATE, TCPDUMP, RM, \
    WHOAMI, PGREP, KILL

CURRENT_CC = None
START_TIME = None

##
# Experiment commands
##
def initializeExperiment(image, cc=DEFAULT_CC):
    global IMAGE, START_TIME
    IMAGE = image
    START_TIME = datetime.datetime.now()
    print '--- starting experiment...'
    print '--- clearing local arp...'
    call([os.path.expanduser('/etalon/bin/arp_clear.sh')])
    print '--- done...'

    print '--- populating physical hosts...'

    del PHYSICAL_NODES[:]  # clear in place
    PHYSICAL_NODES.append('')
    for i in xrange(1, NUM_RACKS + 1):
        PHYSICAL_NODES.append(get_phost_from_id(i))
    print '--- done...'

    print '--- connecting to rpycd...'
    connect_all_rpyc_daemon()
    print '--- done...'

    print '--- setting CC to {}...'.format(cc)
    setCC(cc)
    print '--- done...'

    print '--- building etalon docker image...'
    if not os.path.isfile(DID_BUILD_FN):
        gen_slaves_file(SLAVES_FILE)
        runWriteFile(DOCKER_BUILD, None)
        print '--- done...'

        print '--- copying image to physical hosts...'
        push_docker_image()
        print '--- done...'
    else:
        print '--- skipping (delete %s to force update)...' % (DID_BUILD_FN)

    print '--- launching containers...'
    launch_all_racks(image, blocking=False)
    print '--- done...'

    click_common.initializeClickControl()

    print '--- setting default click buffer sizes and traffic sources...'
    click_common.setConfig({'cc': cc})
    print '--- done...'
    time.sleep(2)
    print '--- done starting experiment...'
    print
    print


def finishExperiment():
    print '--- finishing experiment...'
    print '--- closing final log...'
    click_common.setLog('/tmp/hslog.log')
    print '--- done...'
    print '--- tarring output...'
    tarExperiment()
    print '--- done...'
    print '--- experiment finished'
    if START_TIME is not None:
        print '--- exprtiment took {}'.format(
            datetime.datetime.now() - START_TIME)
    print TIMESTAMP


def getFilesForLater(host, fn, target):
    target_dir = '/'.join(target.split('/')[:-1])
    if not os.path.exists(target_dir):
        os.makedirs(target_dir)
    runWriteFile(SCP % ("root", host, fn, target), None)
    EXPERIMENTS.append(target)


def tarExperiment(compress=False):
    mode = "w"
    if compress:
        mode += ":gz"
    tar = tarfile.open("%s-%s.tar.gz" % (TIMESTAMP, SCRIPT), mode)
    for e in EXPERIMENTS:
        for fn in glob.glob(e):
            # For files in the /tmp directory (i.e., packet logs), place them at
            # the top directory of the archive (i.e., where the rest of the
            # files will be.
            if fn.startswith("/tmp/"):
                arcname = fn[5:]
            else:
                arcname = fn
            tar.add(fn, arcname=arcname)
    tar.close()

    for e in EXPERIMENTS:
        try:
            os.remove(e)
        except OSError:
            # This typically occurs when trying to remove a file not owned by
            # the current user, e.g., Click packet logs stored in /tmp. Make
            # sure to remove packet logs manually if this fails.
            print(("Warning: Unable to remove file: {}\n  Please remove the "
                   "above file manually!").format(e))


##
# rpyc_daemon
##
def connect_all_rpyc_daemon():
    for phost in PHYSICAL_NODES[1:]:
        try:
            if phost not in RPYC_CONNECTIONS:
                RPYC_CONNECTIONS[phost] = rpyc.connect(
                    phost, RPYC_PORT,
                    config={"allow_all_attrs": True, "sync_request_timeout": 1000})
        except:
            raise RuntimeError('Could not connect to: ' + phost)


##
# flowgrind
##
def get_flowgrind_host(h):
    return '%s/%s' % (get_data_ip_from_host(h),
                      get_control_ip_from_host(h))


def gen_big_and_small_flows(seed=92611, rings=1):
    np.random.seed(seed)
    big_bw = 1 / 3.0 * CIRCUIT_BW_Gbps / 8. / rings
    little_bw = 1 / 3.0 * PACKET_BW_Gbps / 8. / NUM_RACKS
    big_nodes = []
    for i in xrange(1, NUM_RACKS + 1):
        big_nodes.append((i, (i % NUM_RACKS) + 1))
    if rings == 2:
        for i in xrange(1, NUM_RACKS + 1):
            big_nodes.append((i, ((i + 1) % NUM_RACKS) + 1))
    flows = []
    psize = 9000
    for s in xrange(1, NUM_RACKS + 1):
        for d in xrange(1, NUM_RACKS + 1):
            t = 0.0
            while t < 2.0:
                src = get_host_from_rack_and_id(
                    s, np.random.randint(1, HOSTS_PER_RACK + 1))
                dst = get_host_from_rack_and_id(
                    d, np.random.randint(1, HOSTS_PER_RACK + 1))
                size = np.random.randint(10 * psize, 100 * psize)
                if (s, d) in big_nodes:
                    size = np.random.randint(1000 * psize, 10000 * psize)
                flows.append({'src': src, 'dst': dst, 'start': t,
                              'size': size, 'response_size': 60,
                              'single': True})
                target_bw = big_bw if (s, d) in big_nodes else little_bw
                t += size / target_bw
    return flows


class Tcpdump(object):
    """ Represents a tcpdump trace running on a remote host. """

    def __init__(self, host, fln_fmt):
        """
        Records a tcpdump trace on a remote host. Uses "fln_fmt" to format the
        output filename. "fln_fmt" should be unique to each experiment.
        """
        assert host != "", "Tcpdump class has not been tested with localhost."
        assert host in PHYSICAL_NODES, \
            ("tcpdump must be run on the physical hosts, but the specified "
             "host is not physical: {}").format(host)
        # The remote host.
        self.host = host
        # Determine the current user.
        self.usr = run(cmd=WHOAMI, fn=None)[1].strip()
        # The path to the remote file in which to store the trace results. Strip
        # the last 7 characters from the end of format. (We assume that the
        # format is from click_common.FN_FORMAT.)
        self.flp_rem = path.join(
            "/home", self.usr,
            "{}-tcpdump-{}.pcap".format(fln_fmt[:-7], self.host))
        # Start the tcpdump trace, running in the background.
        run_on_host(self.host,
                    cmd=("{} &".format(TCPDUMP).format(
                        filepath=self.flp_rem, interface=DATA_EXT_IF)))

    def finish(self):
        """
        Kills the remote tcpdump trace, retrieves the remote pcap file, removes
        it from the remote host, and marks it for inclusion in the results TAR
        file.
        """
        # Sent a SIGINT signal to the tcpdump process.
        run_on_host(self.host,
                    cmd=KILL.format(
                        signal=2,
                        process="`{}`".format(PGREP.format("tcpdump"))))
        # Copy the remote trace file to the local directory.
        flp_lcl = path.basename(self.flp_rem)
        runWriteFile(cmd=SCP % (self.usr, self.host, self.flp_rem, flp_lcl), fn=None)
        # Remove the remote trace file.
        run_on_host(self.host, cmd=RM.format(filepath=self.flp_rem))
        # Mark the local trace file for inclusion in the results TAR file.
        EXPERIMENTS.append(flp_lcl)


def tcpdump_start(fln_fmt):
    """
    Start a remote tcpdump trace on all physical nodes. Returns a list of
    Tcpdump objects running the remote traces. Uses "fln_fmt" to format the
    output filenames. "fln_fmt" should be unique to each experiment.
    """
    return [Tcpdump(host, fln_fmt) for host in PHYSICAL_NODES if host]


def tcpdump_finish(tcpdumps):
    """
    Stops the tcpdump traces being collected by the specified Process objects.
    """
    for tcpdump in tcpdumps:
        tcpdump.finish()


def flowgrind(settings):
    flows = []
    if 'big_and_small' in settings:
        flows = gen_big_and_small_flows()
    if 'big_and_small_two_rings' in settings:
        flows = gen_big_and_small_flows(rings=2)
    else:
        if 'flows' in settings:
            for f in settings['flows']:
                if f['src'][0] == 'r' and f['dst'][0] == 'r':
                    s = int(f['src'][1])
                    d = int(f['dst'][1])
                    for i in xrange(1, HOSTS_PER_RACK + 1):
                        fl = dict(f)
                        fl['src'] = get_host_from_rack_and_id(s, i)
                        fl['dst'] = get_host_from_rack_and_id(d, i)
                        flows.append(fl)
                else:
                    flows.append(f)
    dur_s = settings.get("dur", FLOWGRIND_DEFAULT_DUR_S)
    sample_rate_s = settings.get("sample_rate", FLOWGRIND_DEFAULT_SAMPLE_RATE)
    cmd = '-I -Ts={} -Ys=0 -Gs=q:C:{} -i {} -n {} '.format(
        dur_s, DEFAULT_REQUEST_SIZE, sample_rate_s, len(flows))
    for i, f in enumerate(flows):
        cmd += '-F %d -Hs=%s,d=%s ' % \
               (i, get_flowgrind_host(f['src']), get_flowgrind_host(f['dst']))
    fg_config = click_common.FN_FORMAT % ('flowgrind.config')
    fp = open(fg_config, 'w')
    print("flowgrind cmd: {}".format(cmd))
    fp.write(cmd)
    fp.close()
    cmd = 'flowgrind --configure %s' % fg_config
    print cmd
    EXPERIMENTS.append(fg_config)
    fn = click_common.FN_FORMAT % ('flowgrind')
    print fn
    tcpdumps = tcpdump_start(click_common.FN_FORMAT)
    time.sleep(2)
    runWriteFile(cmd, fn)
    tcpdump_finish(tcpdumps)
    save_counters(click_common.FN_FORMAT % ('flowgrind.counters'))


##
# DFSIOE
##
def dfsioe(host, image):
    fn = click_common.FN_FORMAT % ('dfsioe')
    print fn
    time.sleep(10)
    print 'starting dfsioe...'
    try:
        run_on_host(host, DFSIOE)
    except:
        pass
    print 'done dfsioe...'

    tmp_dir = '/tmp/' + fn.split('.txt')[0]
    for r in xrange(1, NUM_RACKS + 1):
        for h in xrange(1, IMAGE_NUM_HOSTS[image] + 1):
            log_hostname = get_hostname_from_rack_and_id(r, h)
            log_host = get_host_from_rack_and_id(r, h)
            getFilesForLater(log_hostname, "/usr/local/hadoop/logs/*",
                             tmp_dir + '/' + log_host + '-logs/')

    r, h = get_rack_and_id_from_host(host)
    log_hostname = get_hostname_from_rack_and_id(r, h)
    getFilesForLater(log_hostname, '~/HiBench/report', tmp_dir)

    save_counters(click_common.FN_FORMAT % ('dfsioe.counters'))


##
# Byte Counters
##
def save_counters(fn):
    fp = open(fn, 'w')
    counter_data = click_common.getCounters()
    fp.write(str(counter_data))
    fp.close()
    EXPERIMENTS.append(fn)


##
# Running shell commands
##
def run(cmd, fn):
    print("host: local , cmd: {}".format(cmd))
    def preexec():  # don't forward signals
        os.setpgrp()

    out = ""
    p = Popen(cmd, shell=True, stdout=PIPE, stderr=STDOUT,
              preexec_fn=preexec)
    while True:
        line = p.stdout.readline()  # this will block
        if not line:
            break
        if not fn:
            sys.stdout.write(line)
        out += line
    rc = p.poll()
    while rc is None:
        rc = p.poll()
    if rc != 0:
        raise Exception("subprocess.CalledProcessError: Command '%s'"
                        "returned non-zero exit status %s\n"
                        "output was: %s" % (cmd, rc, out))
    return (rc, out)


def runWriteFile(cmd, fn):
    if fn:
        EXPERIMENTS.append(fn)
    try:
        out = run(cmd, fn)[1]
    except Exception, e:
        print e
        out = str(e)
    if fn:
        f = open(fn, 'w')
        f.write(out)
        f.close()


##
# Congestion Control
##
def set_cc_host(phost, cc):
    run_on_host(phost, SET_CC.format(cc=cc))


def setCC(cc):
    global CURRENT_CC
    ts = []
    for phost in PHYSICAL_NODES[1:]:
        ts.append(threading.Thread(target=set_cc_host,
                                   args=(phost, cc)))
        ts[-1].start()
    map(lambda t: t.join(), ts)
    # If the CC mode was previously configured to something else, then restart
    # the cluster.
    if CURRENT_CC is not None and cc != CURRENT_CC:
        launch_all_racks(IMAGE, blocking=False)
    CURRENT_CC = cc


##
# Docker
##
def push_docker_image_to_host(phost):
    runWriteFile(SCP_TO % (DOCKER_LOCAL_IMAGE_PATH,
                           phost, DOCKER_REMOTE_IMAGE_PATH), None)
    run_on_host(phost, DOCKER_LOAD)


def push_docker_image():
    print 'saving docker image...'
    runWriteFile(DOCKER_SAVE, None)
    print 'done...'
    print 'copying to physical hosts...'
    ts = []
    for phost in PHYSICAL_NODES[1:]:
        ts.append(threading.Thread(target=push_docker_image_to_host,
                                   args=(phost,)))
        ts[-1].start()
    map(lambda t: t.join(), ts)
    runWriteFile('touch %s' % DID_BUILD_FN, None)
    print 'done...'


def run_on_host(host, cmd, timeout_s=0):
    print("host: {} , cmd: {}".format(host, cmd))
    if host in PHYSICAL_NODES:
        func = RPYC_CONNECTIONS[get_phost_from_host(host)].root.run_fully
    else:
        if 'arp' in cmd or 'ping' in cmd:
            func = lambda c: RPYC_CONNECTIONS[
                get_phost_from_host(host)].root.run_fully_host_ns(
                    host, c, timeout_s, interval_s=1)
        else:
            if host[0] == 'h':
                host = host[1:]
            func = lambda c: RPYC_CONNECTIONS[
                get_phost_from_host(host)].root.run_fully_host(host, c)
    return func(cmd)


def launch(phost, image, host_id):
    my_id = '%d%d' % (get_phost_id(phost), host_id)
    cpu_lim = IMAGE_CPU[image]

    my_cmd = '/bin/sh -c ' + IMAGE_CMD[IMAGE_SETUP[image]['h' + my_id]]

    # bind to specific CPU
    cpus = CPU_SET
    cpus = str((host_id % (CPU_COUNT - 1)) + 1)
    my_cmd = my_cmd.format(cpu=cpus)

    run_cmd = IMAGE_DOCKER_RUN[image]
    run_on_host(phost, run_cmd.format(image=DOCKER_IMAGE, hosts_file=HOSTS_FILE,
                                      hid=my_id, FQDN=FQDN, cpu_set=cpus,
                                      cpu_limit=cpu_lim, cmd=my_cmd))
    run_on_host(phost, PIPEWORK.format(ext_if=DATA_EXT_IF, int_if=DATA_INT_IF,
                                       net=DATA_NET, rack=get_phost_id(phost),
                                       hid=host_id))
    if not IMAGE_SKIP_TC[image]:
        run_on_host(phost, TC.format(int_if=DATA_INT_IF,
                                     hid=my_id, rate=DATA_RATE_Gbps_TDF))

    run_on_host(my_id, SWITCH_PING, timeout_s=600)
    smac = run_on_host(my_id, GET_SWITCH_MAC).strip()

    # ARP poison. Set the MAC address of all the other emulated hosts to the MAC
    # of the switch. This will cause all traffic sent to the other emulated
    # hosts to be sent to the switch instead, where it will be processed by the
    # hybrid router.
    for i in xrange(1, NUM_RACKS + 1):
        if i == get_phost_id(phost):
            continue
        for j in xrange(1, HOSTS_PER_RACK + 1):
            dst_id = '%d%d.%s' % (i, j, FQDN)
            run_on_host(my_id, ARP_POISON.format(hid=dst_id, switch_mac=smac))

    run_on_host(phost,
                PIPEWORK.format(ext_if=CONTROL_EXT_IF, int_if=CONTROL_INT_IF,
                                net=CONTROL_NET, rack=get_phost_id(phost),
                                hid=host_id))
    if not IMAGE_SKIP_TC[image]:
        run_on_host(phost, TC.format(int_if=CONTROL_INT_IF, hid=my_id,
                                     rate=CONTROL_RATE_Gbps_TDF))


def launch_rack(phost, image, blocking=True):
    ts = []
    for i in xrange(1, IMAGE_NUM_HOSTS[image] + 1):
        if blocking:
            launch(phost, image, i)
        else:
            ts.append(threading.Thread(target=launch, args=(phost, image, i)))
            ts[-1].start()
    map(lambda t: t.join(), ts)


def launch_all_racks(image, blocking=True):
    gen_hosts_file(HOSTS_FILE)
    for phost in PHYSICAL_NODES[1:]:
        try:
            run_on_host(phost, REMOVE_HOSTS_FILE)
        except:
            pass
        try:
            run_on_host(phost, DOCKER_CLEAN)
        except:
            pass

    ts = []
    for phost in PHYSICAL_NODES[1:]:
        runWriteFile(SCP_TO % (HOSTS_FILE, phost, HOSTS_FILE), None)
        if blocking:
            launch_rack(phost, image, blocking)
        else:
            ts.append(threading.Thread(target=launch_rack, args=(phost, image, blocking)))
            ts[-1].start()
    map(lambda t: t.join(), ts)

    num_hosts = IMAGE_NUM_HOSTS[image]
    for r in xrange(1, NUM_RACKS + 1):
        for h in xrange(1, num_hosts + 1):
            ip = get_control_ip_from_host(get_host_from_rack_and_id(r, h))
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            port = FLOWGRIND_PORT if 'flowgrind' in image else HDFS_PORT
            while sock.connect_ex((ip, port)):
                time.sleep(1)
            sock.close()
