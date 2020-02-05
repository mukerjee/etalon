
from os import path
import socket
import sys
# Directory containing this program.
PROGDIR = path.dirname(path.realpath(__file__))
# For python_config.
sys.path.insert(0, path.join(PROGDIR, "..", "etc"))
import time

import common
from python_config import NUM_RACKS, TIMESTAMP, SCRIPT, TDF, EXPERIMENTS, \
    CLICK_ADDR, CLICK_PORT, CLICK_BUFFER_SIZE, DEFAULT_CIRCUIT_CONFIG, \
    CIRCUIT_LATENCY_s_TDF, RECONFIG_DELAY_us, PACKET_BW_Gbps_TDF, DEFAULT_CC

CLICK_SOCKET = None
FN_FORMAT = ''


def initializeClickControl():
    global CLICK_SOCKET
    print '--- connecting to click socket...'
    CLICK_SOCKET = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    CLICK_SOCKET.connect((CLICK_ADDR, CLICK_PORT))
    CLICK_SOCKET.recv(CLICK_BUFFER_SIZE)
    print '--- done...'


def clickWriteHandler(element, handler, value):
    message = "WRITE %s.%s %s\n" % (element, handler, value)
    print message.strip()
    CLICK_SOCKET.send(message)
    print CLICK_SOCKET.recv(CLICK_BUFFER_SIZE).strip()


def clickReadHandler(element, handler):
    message = "READ %s.%s\n" % (element, handler)
    print message.strip()
    CLICK_SOCKET.send(message)
    # The return value is three lines. The last line is the actual value.
    data = int(CLICK_SOCKET.recv(CLICK_BUFFER_SIZE).strip().split("\n")[-1])
    print data
    return data


def setLog(log):
    print 'changing log fn to %s' % log
    clickWriteHandler('hsl', 'openLog', log)
    if log != '/tmp/hslog.log':
        EXPERIMENTS.append(log)
    time.sleep(0.1)


def disableLog():
    print 'disabling packet logging'
    clickWriteHandler('hsl', 'disableLog', '')
    time.sleep(0.1)


def setQueueCap(cap, which):
    clickWriteHandler('runner', '{}_queue_capacity'.format(which), cap)


def setSmallQueueCap(cap):
    setQueueCap(cap, which="small")


def setBigQueueCap(cap):
    setQueueCap(cap, which="big")


def setEstimateTrafficSource(source):
    clickWriteHandler('traffic_matrix', 'setSource', source)


def setInAdvance(in_advance):
    clickWriteHandler('runner', 'setInAdvance', in_advance)


def setQueueResize(b):
    if b:
        clickWriteHandler('runner', 'setDoResize', 'true')
    else:
        clickWriteHandler('runner', 'setDoResize', 'false')
    time.sleep(0.1)


def getCounters():
    print "--- getting Click counters..."
    circuit_bytes = []
    packet_up_bytes = []
    packet_down_bytes = []
    for i in xrange(1, NUM_RACKS + 1):
        circuit_bytes.append(
            clickReadHandler('hybrid_switch/circuit_link%d/lu' % (i),
                             'total_bytes'))
        packet_up_bytes.append(
            clickReadHandler('hybrid_switch/packet_up_link%d/lu' % (i),
                             'total_bytes'))
        packet_down_bytes.append(
            clickReadHandler('hybrid_switch/ps/packet_link%d/lu' % (i),
                             'total_bytes'))
    print "--- done..."
    return (circuit_bytes, packet_up_bytes, packet_down_bytes)


def clearCounters():
    print("--- clearing Click counters...")
    for i in xrange(1, NUM_RACKS + 1):
        for j in xrange(1, NUM_RACKS + 1):
            clickWriteHandler('hybrid_switch/q%d%d/q' % (i, j),
                              'clear', "")
        clickWriteHandler('hybrid_switch/circuit_link%d/lu' % (i),
                          'clear', "")
        clickWriteHandler('hybrid_switch/packet_up_link%d/lu' % (i),
                          'clear', "")
        clickWriteHandler('hybrid_switch/ps/packet_link%d/lu' % (i),
                          'clear', "")
    clickWriteHandler('traffic_matrix', 'clear', "")
    print("--- done...")


def divertACKs(divert):
    clickWriteHandler('divert_acks', 'switch', 1 if divert else 0)


def setCircuitLinkDelay(delay):
    for i in xrange(1, NUM_RACKS + 1):
        clickWriteHandler('hybrid_switch/circuit_link%d/lu' % (i),
                          'latency', delay)


def setPacketLinkBandwidth(bw):
    for i in xrange(1, NUM_RACKS + 1):
        clickWriteHandler('hybrid_switch/packet_up_link%d/lu' % (i),
                          'bandwidth', '%.1fGbps' % bw)
        clickWriteHandler('hybrid_switch/ps/packet_link%d/lu' % (i),
                          'bandwidth', '%.1fGbps' % bw)


def setSolsticeThresh(thresh):
    clickWriteHandler('sol', 'setThresh', thresh)


##
# Scheduling
##
def enableSolstice():
    clickWriteHandler('sol', 'setEnabled', 'true')
    time.sleep(0.1)


def disableSolstice():
    clickWriteHandler('sol', 'setEnabled', 'false')
    time.sleep(0.1)


def disableCircuit():
    setFixedSchedule('1 20000 %s' % (('-1/' * NUM_RACKS)[:-1]))


def setStrobeSchedule(num_racks=NUM_RACKS, night_len_us=20, day_len_us=180):
    # Configuration that turns off all the circuit links. Remove trailing '/'.
    off_config = ('-1/' * num_racks)[:-1]
    # Day len, day config, night len, off config
    config_s = '%d %s %d %s '
    # The first element is the number of configurations. There are num_racks - 1
    # configurations because there does not need to be a configuration where the
    # racks connect to themselves. Each configuration actually contains both a
    # day and a night.
    schedule = '%d ' % ((num_racks - 1) * 2)
    for i in xrange(num_racks - 1):
        day_config = ''
        for j in xrange(num_racks):
            day_config += '%d/' % ((i + 1 + j) % num_racks)
        # Remove trailing '/'.
        day_config = day_config[:-1]
        schedule += config_s % (day_len_us, day_config, night_len_us, off_config)
    # Remove trailing space.
    schedule = schedule[:-1]
    setFixedSchedule(schedule)


def setFakeStrobeSchedule(num_racks_fake=NUM_RACKS, night_len_us=20, day_len_us=180):
    """
    Set a schedule that, from the point of view of one node, appears to be a
    strobe schedule on a testbed with "num_racks_fake" racks.

    E.g., for setFakeSchedule(8, 3600, 400) will create this schedule:
        "14
            3600 1/2/0 400 -1/-1/-1   # 1
            3600 1/2/0 400 -1/-1/-1   # 2
            3600 1/2/0 400 -1/-1/-1   # 3
            3600 1/2/0 400 -1/-1/-1   # 4
            3600 1/2/0 400 -1/-1/-1   # 5
            3600 1/2/0 400 -1/-1/-1   # 6
            3600 2/0/1 400 -1/-1/-1"  # 7
    This schedule mimics an eight-rack cluster where each rack gets a circuit to
    every other rack for 180us (under time dilation). Values are the srcs,
    indices are the dsts. E.g., 1/2/0 means that 1->0, 2->1, and 0->2. Day/night
    pairs 1-6 are the "other" circuits. 7 is the circuit that our test flow will
    traverse. There are only seven configurations total for an eight-rack
    cluster because we do not need a configuration where each of the racks
    connects to itself.
    """
    assert num_racks_fake >= 3, \
        ("Can only fake testbeds with more than 3 racks, but "
         "\"num_racks_fake\" = {}").format(num_racks_fake)
    num_parts = num_racks_fake - 1
    schedule = "{} ".format(num_parts * 2)
    schedule += "{} 1/2/0 {} -1/-1/-1 ".format(day_len_us, night_len_us) * (num_parts - 1)
    schedule += "{} 2/0/1 {} -1/-1/-1".format(day_len_us, night_len_us)
    setFixedSchedule(schedule)


def setCircuitSchedule(configuration):
    setFixedSchedule('1 %d %s' % (20 * TDF * 10 * 10, configuration))


def setFixedSchedule(schedule):
    disableSolstice()
    clickWriteHandler('runner', 'setSchedule', schedule)
    time.sleep(0.1)


def setEceEnabled(enabled):
    """ Enable/disable ECE marking. """
    clickWriteHandler('ecem', 'enabled', 'true' if enabled else 'false')
    time.sleep(0.1)


def setEcnEnabled(enabled):
    """ Enable/disable threshold-based ECN marking. """
    val = 'true' if enabled else 'false'
    for src in xrange(1, NUM_RACKS + 1):
        for dst in xrange(1, NUM_RACKS + 1):
            clickWriteHandler('hybrid_switch/q{}{}/q'.format(src, dst),
                              'marking_enabled', val)
    time.sleep(0.1)


def setEcnThresh(thresh):
    """ Set the ECN marking threshold. """
    for src in xrange(1, NUM_RACKS + 1):
        for dst in xrange(1, NUM_RACKS + 1):
            clickWriteHandler('hybrid_switch/q{}{}/q'.format(src, dst),
                              'marking_threshold', thresh)
    time.sleep(0.1)


def setConfig(config):
    global FN_FORMAT
    c = {'type': 'normal', 'small_queue_cap': 16, 'big_queue_cap': 128,
         'traffic_source': 'QUEUE', 'queue_resize': False,
         'in_advance': 12000, 'cc': DEFAULT_CC, 'packet_log': True,
         'divert_acks': False, 'circuit_link_delay': CIRCUIT_LATENCY_s_TDF,
         'packet_link_bandwidth': PACKET_BW_Gbps_TDF, 'hdfs': False,
         'thresh': 1000000, 'night_len_us': RECONFIG_DELAY_us * TDF,
         'day_len_us': RECONFIG_DELAY_us * TDF * 9}

    c.update(config)
    clearCounters()
    setSmallQueueCap(c['small_queue_cap'])
    setBigQueueCap(c['big_queue_cap'])
    setEstimateTrafficSource(c['traffic_source'])
    setInAdvance(c['in_advance'])
    common.setCC(c['cc'])
    setSolsticeThresh(c['thresh'])

    t = c['type']
    if t == 'normal':
        enableSolstice()
    if t == 'no_circuit':
        disableCircuit()
    if t == 'strobe':
        setStrobeSchedule(NUM_RACKS, night_len_us=c['night_len_us'],
                          day_len_us=c['day_len_us'])
    if t == 'fake_strobe':
        setFakeStrobeSchedule(num_racks_fake=c['num_racks_fake'],
                              night_len_us=c['night_len_us'],
                              day_len_us=c['day_len_us'])
    if t == 'circuit':
        setCircuitSchedule(DEFAULT_CIRCUIT_CONFIG)
    if t == 'fixed':
        setFixedSchedule(c['fixed_schedule'])

    # Enable threshold-based ECN marking only if it is explicitly configured.
    ecn_enabled = 'ecn' in c
    if ecn_enabled:
        setEcnThresh(c['ecn'])
    setEcnEnabled(ecn_enabled)
    # If using reTCP, then enable ECE marking.
    setEceEnabled(c['cc'] == 'retcp')

    # Do this after passing through manual queue sizes and setting the ECN
    # marking threshold.
    setQueueResize(c['queue_resize'])

    divertACKs(c['divert_acks'])
    setCircuitLinkDelay(c['circuit_link_delay'])
    setPacketLinkBandwidth(c['packet_link_bandwidth'])

    FN_FORMAT = "{}-{}-{}-{}-{}-{}-{}-{}-{}-{}-{}-{}-".format(
        TIMESTAMP, SCRIPT, t,
        c["small_queue_cap"],
        c["big_queue_cap"],
        c["traffic_source"],
        c["queue_resize"],
        c["in_advance"],
        c["cc"],
        c["circuit_link_delay"],
        c["packet_link_bandwidth"],
        c["hdfs"])
    if t in ["fake_strobe", "strobe"]:
        FN_FORMAT += "{}-{}-".format(c["night_len_us"], c["day_len_us"])
    if t == "fake_strobe":
        FN_FORMAT += "{}-".format(c["num_racks_fake"])

    FN_FORMAT += '%s.txt'
    if config and c['packet_log']:
        setLog('/tmp/' + FN_FORMAT % 'click')
    if not c['packet_log']:
        disableLog()
