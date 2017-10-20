import socket
import time
from globals import NUM_RACKS, TIMESTAMP, SCRIPT, TDF

TCP_IP = 'localhost'
TCP_PORT = 1239
BUFFER_SIZE = 1024
CLICK_SOCKET = None

CURRENT_CONFIG = {}
FN_FORMAT = ''


##
# Running click commands
##    
def initializeClickControl():
    global CLICK_SOCKET
    print '--- connecting to click socket...'
    CLICK_SOCKET = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    CLICK_SOCKET.connect((TCP_IP, TCP_PORT))
    CLICK_SOCKET.recv(BUFFER_SIZE)
    print '--- done...'


def clickWriteHandler(element, handler, value):
    message = "WRITE %s.%s %s\n" % (element, handler, value)
    print message.strip()
    CLICK_SOCKET.send(message)
    print CLICK_SOCKET.recv(BUFFER_SIZE).strip()


def clickReadHandler(element, handler):
    message = "READ %s.%s\n" % (element, handler)
    print message.strip()
    CLICK_SOCKET.send(message)
    data = CLICK_SOCKET.recv(BUFFER_SIZE).strip()
    print data
    return data


def setQueueSize(size):
    for i in xrange(1, NUM_RACKS+1):
        for j in xrange(1, NUM_RACKS+1):
            clickWriteHandler('hybrid_switch/q%d%d/q' % (i, j),
                              'capacity', size)


def setEstimateTrafficSource(source):
    clickWriteHandler('traffic_matrix', 'setSource', source)


def setQueueResize(b):
    if b:
        clickWriteHandler('runner', 'setDoResize', 'true')
    else:
        clickWriteHandler('runner', 'setDoResize', 'false')
    time.sleep(0.1)


def enableSolstice():
    clickWriteHandler('sol', 'setEnabled', 'true')
    time.sleep(0.1)


def disableSolstice():
    clickWriteHandler('sol', 'setEnabled', 'false')
    time.sleep(0.1)


def disableCircuit():
    disableSolstice()
    off_sched = '1 20000 %s' % (('-1/' * NUM_RACKS)[:-1])
    clickWriteHandler('runner', 'setSchedule', off_sched)
    time.sleep(0.1)


def setStrobeSchedule():
    disableSolstice()
    schedule = '%d ' % ((NUM_RACKS-1)*2)
    for i in xrange(NUM_RACKS-1):
        configstr = '%d %s %d %s '
        night_len = 20 * TDF
        off_config = ('-1/' * NUM_RACKS)[:-1]
        duration = night_len * 9  # night_len * duty_cycle
        configuration = ''
        for j in xrange(NUM_RACKS):
            configuration += '%d/' % ((i + 1 + j) % NUM_RACKS)
        configuration = configuration[:-1]
        schedule += (configstr % (duration, configuration, night_len,
                                  off_config))
    schedule = schedule[:-1]
    clickWriteHandler('runner', 'setSchedule', schedule)
    time.sleep(0.1)


def setConfig(config):
    global CURRENT_CONFIG, FN_FORMAT
    CURRENT_CONFIG = {'type': 'normal', 'buffer_size': 40,
                      'traffic_source': 'QUEUE', 'queue_resize': False}
    CURRENT_CONFIG.update(config)
    c = CURRENT_CONFIG
    setQueueResize(False)  # let manual queue sizes be passed through first
    setQueueSize(c['buffer_size'])
    setEstimateTrafficSource(c['traffic_source'])
    setQueueResize(c['queue_resize'])
    t = c['type']
    if t == 'normal':
        enableSolstice()
    if t == 'no_circuit':
        disableCircuit()
    if t == 'strobe':
        setStrobeSchedule()
    FN_FORMAT = '%s-%s-%s-%d-%s-%s-' % (TIMESTAMP, SCRIPT, t, c['buffer_size'],
                                        c['traffic_source'], c['queue_resize'])
    FN_FORMAT += '%s.txt'
