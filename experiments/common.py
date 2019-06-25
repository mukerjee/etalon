import time
import threading
import os
import tarfile
import socket
import glob
import rpyc
import numpy as np
import click_common
from subprocess import call, PIPE, STDOUT, Popen
import sys
sys.path.insert(0, '/etalon/etc')
from python_config import NUM_RACKS, HOSTS_PER_RACK, TIMESTAMP, SCRIPT, \
    EXPERIMENTS, PHYSICAL_NODES, RPYC_CONNECTIONS, RPYC_PORT, CIRCUIT_BW, \
    PACKET_BW, FQDN, DATA_NET, CONTROL_NET, DFSIOE, get_phost_from_host, \
    get_phost_id, IMAGE_CPU, CPU_COUNT, CPU_SET, IMAGE_CMD, IMAGE_SETUP, \
    DOCKER_RUN, DOCKER_IMAGE, PIPEWORK, DATA_EXT_IF, DATA_INT_IF, \
    IMAGE_SKIP_TC, TC, DATA_RATE, SWITCH_PING, GET_SWITCH_MAC, ARP_POISON, \
    CONTROL_EXT_IF, CONTROL_INT_IF, CONTROL_RATE, DOCKER_CLEAN, \
    IMAGE_NUM_HOSTS, DOCKER_BUILD, SET_CC, get_host_from_rack_and_id, SCP, \
    get_data_ip_from_host, get_control_ip_from_host, FLOWGRIND_PORT, \
    HDFS_PORT, DOCKER_SAVE, SCP_TO, DOCKER_LOCAL_IMAGE_PATH, \
    DOCKER_REMOTE_IMAGE_PATH, DOCKER_LOAD, get_phost_from_id, DID_BUILD_FN, \
    gen_hosts_file, HOSTS_FILE, IMAGE_DOCKER_RUN, REMOVE_HOSTS_FILE, \
    gen_slaves_file, SLAVES_FILE, get_hostname_from_rack_and_id, \
    get_rack_and_id_from_host

THREAD_LOCK = threading.Lock()
CURRENT_CC = ''


##
# Experiment commands
##
def initializeExperiment(image):
    global IMAGE
    IMAGE = image
    print '--- starting experiment...'
    print '--- clearing local arp...'
    call([os.path.expanduser('/etalon/bin/arp_clear.sh')])
    print '--- done...'

    print '--- populating physical hosts...'

    del PHYSICAL_NODES[:]  # clear in place
    PHYSICAL_NODES.append('')
    for i in xrange(1, NUM_RACKS+1):
        PHYSICAL_NODES.append(get_phost_from_id(i))
    print '--- done...'

    print '--- connecting to rpycd...'
    connect_all_rpyc_daemon()
    print '--- done...'

    print '--- setting CC to reno...'
    setCC('reno')
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
    launch_all_racks(image)
    print '--- done...'

    click_common.initializeClickControl()

    print '--- setting default click buffer sizes and traffic sources...'
    click_common.setConfig({})
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
    print TIMESTAMP


def getFilesForLater(host, fn, target):
    target_dir = '/'.join(target.split('/')[:-1])
    if not os.path.exists(target_dir):
        os.makedirs(target_dir)
    runWriteFile(SCP % (host, fn, target), None)
    EXPERIMENTS.append(target)


def tarExperiment():
    tar = tarfile.open("%s-%s.tar.gz" % (TIMESTAMP, SCRIPT), "w:gz")
    for e in EXPERIMENTS:
        for fn in glob.glob(e):
            tar.add(fn)
    tar.close()

    for e in EXPERIMENTS:
        if 'tmp' not in e:
            os.remove(e)


##
# rpyc_daemon
##
def connect_all_rpyc_daemon():
    bad_hosts = []
    for phost in PHYSICAL_NODES[1:]:
        try:
            if phost not in RPYC_CONNECTIONS:
                RPYC_CONNECTIONS[phost] = rpyc.connect(phost, RPYC_PORT, config={"allow_all_attrs": True})
        except:
            print 'could not connect to ' + phost
            bad_hosts.append(phost)
    map(lambda x: PHYSICAL_NODES.remove(x), bad_hosts)


##
# flowgrind
##
def get_flowgrind_host(h):
    return '%s/%s' % (get_data_ip_from_host(h),
                      get_control_ip_from_host(h))


def gen_big_and_small_flows(seed=92611, rings=1):
    np.random.seed(seed)
    big_bw = 1/3.0 * CIRCUIT_BW / 8.0 / rings
    little_bw = 1/3.0 * PACKET_BW / 8.0 / NUM_RACKS
    big_nodes = []
    for i in xrange(1, NUM_RACKS+1):
        big_nodes.append((i, (i % NUM_RACKS) + 1))
    if rings == 2:
        for i in xrange(1, NUM_RACKS+1):
            big_nodes.append((i, ((i+1) % NUM_RACKS) + 1))
    flows = []
    psize = 9000
    for s in xrange(1, NUM_RACKS+1):
        for d in xrange(1, NUM_RACKS+1):
            t = 0.0
            while t < 2.0:
                src = get_host_from_rack_and_id(
                    s, np.random.randint(1, HOSTS_PER_RACK+1))
                dst = get_host_from_rack_and_id(
                    d, np.random.randint(1, HOSTS_PER_RACK+1))
                size = np.random.randint(10 * psize, 100 * psize)
                if (s, d) in big_nodes:
                    size = np.random.randint(1000 * psize, 10000 * psize)
                flows.append({'src': src, 'dst': dst, 'start': t,
                              'size': size, 'response_size': 60,
                              'single': True})
                target_bw = big_bw if (s, d) in big_nodes else little_bw
                t += size / target_bw
    return flows


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
                    for i in xrange(1, HOSTS_PER_RACK+1):
                        fl = dict(f)
                        fl['src'] = get_host_from_rack_and_id(s, i)
                        fl['dst'] = get_host_from_rack_and_id(d, i)
                        flows.append(fl)
                else:
                    flows.append(f)
    cmd = '-n %s ' % len(flows)
    for i, f in enumerate(flows):
        if 'time' not in f:
            f['time'] = 2.0
        if 'start' not in f:
            f['start'] = 0
        if 'size' not in f:
            f['size'] = 8948
        cmd += '-F %d ' % (i)
        if 'fg_report_interval' in settings:
            cmd += '-i %f ' % (settings['fg_report_interval'])
        else:
            cmd += '-Q -i 2 '
        cmd += '-Hs=%s,d=%s -Ts=%f -Ys=%f -Gs=q:C:%d ' % \
            (get_flowgrind_host(f['src']), get_flowgrind_host(f['dst']),
             f['time'], f['start'], f['size'])
        if 'response_size' in f:
            cmd += '-Gs=p:C:%d ' % f['response_size']
        if 'single' in f:
            cmd += '-Z 1 '
    cmd += '-I '
    fg_config = click_common.FN_FORMAT % ('flowgrind.config')
    fp = open(fg_config, 'w')
    fp.write(cmd)
    fp.close()
    cmd = 'flowgrind --configure %s' % fg_config
    print cmd
    EXPERIMENTS.append(fg_config)
    fn = click_common.FN_FORMAT % ('flowgrind')
    print fn
    runWriteFile(cmd, fn)
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
    for r in xrange(1, NUM_RACKS+1):
        for h in xrange(1, IMAGE_NUM_HOSTS[image]+1):
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
    if CURRENT_CC and cc != CURRENT_CC:
        launch_all_racks(IMAGE)
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


def run_on_host(host, cmd):
    print("host: {} , cmd: {}".format(host, cmd))
    if host in PHYSICAL_NODES:
        func = RPYC_CONNECTIONS[get_phost_from_host(host)].root.run
    else:
        if 'arp' in cmd or 'ping' in cmd:
            func = lambda c: RPYC_CONNECTIONS[
                get_phost_from_host(host)].root.ns_run(host, c)
        else:
            if host[0] == 'h':
                host = host[1:]
            func = lambda c: RPYC_CONNECTIONS[
                get_phost_from_host(host)].root.run_host(host, c)
    return func(cmd)


def launch(phost, image, host_id):
    my_id = '%d%d' % (get_phost_id(phost), host_id)
    cpu_lim = IMAGE_CPU[image]

    my_cmd = '/bin/sh -c ' + IMAGE_CMD[IMAGE_SETUP[image]['h' + my_id]]

    # bind to specific CPU
    cpus = CPU_SET
    cpus = str((host_id % (CPU_COUNT-1)) + 1)
    my_cmd = my_cmd.format(cpu=cpus)

    run_cmd = IMAGE_DOCKER_RUN[image]
    run_on_host(phost, run_cmd.format(image=DOCKER_IMAGE, hosts_file=HOSTS_FILE,
                                      id=my_id, FQDN=FQDN, cpu_set=cpus,
                                      cpu_limit=cpu_lim, cmd=my_cmd))
    run_on_host(phost, PIPEWORK.format(ext_if=DATA_EXT_IF, int_if=DATA_INT_IF,
                                       net=DATA_NET, rack=get_phost_id(phost),
                                       id=host_id))
    if not IMAGE_SKIP_TC[image]:
        run_on_host(phost, TC.format(int_if=DATA_INT_IF,
                                     id=my_id, rate=DATA_RATE))

    run_on_host(my_id, SWITCH_PING)
    smac = run_on_host(my_id, GET_SWITCH_MAC).strip()

    for i in xrange(1, NUM_RACKS+1):
        if i == get_phost_id(phost):
            continue
        for j in xrange(1, HOSTS_PER_RACK+1):
            dst_id = '%d%d.%s' % (i, j, FQDN)
            run_on_host(my_id, ARP_POISON.format(id=dst_id, switch_mac=smac))

    run_on_host(phost,
                PIPEWORK.format(ext_if=CONTROL_EXT_IF, int_if=CONTROL_INT_IF,
                                net=CONTROL_NET, rack=get_phost_id(phost),
                                id=host_id))
    if not IMAGE_SKIP_TC[image]:
        run_on_host(phost, TC.format(int_if=CONTROL_INT_IF, id=my_id,
                                     rate=CONTROL_RATE))


def launch_rack(phost, image, blocking=True):
    ts = []
    for i in xrange(1, IMAGE_NUM_HOSTS[image]+1):
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
            launch_rack(phost, image)
        else:
            ts.append(threading.Thread(target=launch_rack, args=(phost, image)))
            ts[-1].start()
    map(lambda t: t.join(), ts)

    num_hosts = IMAGE_NUM_HOSTS[image]
    for r in xrange(1, NUM_RACKS+1):
        for h in xrange(1, num_hosts+1):
            ip = get_control_ip_from_host(get_host_from_rack_and_id(r, h))
            sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
            port = FLOWGRIND_PORT if 'flowgrind' in image else HDFS_PORT
            while sock.connect_ex((ip, port)):
                time.sleep(1)
            sock.close()
