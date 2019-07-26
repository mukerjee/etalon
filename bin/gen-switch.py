#!/usr/bin/env python

from os import path
import sys
# Directory containing this program.
PROGDIR = path.dirname(path.realpath(__file__))
# For python_config.
sys.path.insert(0, path.join(PROGDIR, '..', 'etc'))

from python_config import DATA_EXT_IF, NUM_RACKS, HOSTS_PER_RACK, \
    CIRCUIT_BW_Gbps_TDF, PACKET_BW_Gbps_TDF, CIRCUIT_LATENCY_s_TDF, \
    PACKET_LATENCY_s_TDF, RECONFIG_DELAY_us, TDF, CLICK_PORT, \
    get_data_ip_from_host, get_phost_from_id, get_host_from_rack_and_id


print
print '// For more information, see etalon/bin/gen-switch.py.'
print
print 'define($DEVNAME %s)' % DATA_EXT_IF
print 'define($NUM_RACKS %s)' % NUM_RACKS
print

# defining all host and vhost (data_net) IPs
k = 0
ip_def = 'define('
for i in xrange(1, NUM_RACKS+1):
    ip_def += '$IP%d %s, ' % (i, get_data_ip_from_host(get_phost_from_id(i)))
    for j in xrange(1, HOSTS_PER_RACK+1):
        ip_str = '$IP%d%d' % (i, j)
        ip = get_data_ip_from_host(get_host_from_rack_and_id(i, j))
        ip_def += '%s %s, ' % (ip_str, ip)
        k += 1
        if k == 4:
            ip_def = ip_def[:-1]
            ip_def += '\n       '
            k = 0
ip_def = ip_def.strip()[:-1] + ')'

print ip_def
print

# all other params (set in ../etc/python_config.py)
print 'define ($CIRCUIT_BW_Gbps_TDF %.1fGbps, $PACKET_BW_Gbps_TDF %.1fGbps)' % (
    CIRCUIT_BW_Gbps_TDF, PACKET_BW_Gbps_TDF)
print

print 'define ($PACKET_LATENCY_s_TDF %s)' % PACKET_LATENCY_s_TDF
print 'define ($CIRCUIT_LATENCY_s_TDF %s)' % CIRCUIT_LATENCY_s_TDF
print 'define ($BIG_BUFFER_SIZE %s, $SMALL_BUFFER_SIZE %s)' % (
    128, 16)
print

print 'define ($RECONFIG_DELAY_us %d)' % RECONFIG_DELAY_us
print 'define ($TDF %d)' % (int(TDF))
print

# mappings of elements to cores
print 'StaticThreadSched(in 0,'
print '                  traffic_matrix 1,'
print '                  sol 2,'
print '                  runner 3,'
j = 4
k = 0
for i in xrange(1, NUM_RACKS+1):
    print '                  hybrid_switch/circuit_link%d %d,' % (i, j)
    k += 1
    j += 1
k = 0
for i in xrange(1, NUM_RACKS+1):
    print '                  hybrid_switch/packet_up_link%d %d,' % (i, j)
    k += 1
    if k == 4:
        j += 1
        k = 0
k = 0
for i in xrange(1, NUM_RACKS+1):
    print '                  hybrid_switch/ps/packet_link%d %d,' % (i, j)
    k += 1
    if k == 4:
        j += 1
        k = 0
print ')'
print

print 'ControlSocket("TCP", %d)' % (CLICK_PORT)
print

# the three control elements (see the paper)
print 'traffic_matrix :: EstimateTraffic($NUM_RACKS, SOURCE QUEUE)'
print 'sol :: Solstice($NUM_RACKS, $CIRCUIT_BW_Gbps_TDF, $PACKET_BW_Gbps_TDF, ' \
    '$RECONFIG_DELAY_us, $TDF)'
print 'runner :: RunSchedule($NUM_RACKS, RESIZE false)'
print

# entry and exit points
print 'in :: FromDPDKDevice(0, MTU 9000)'
print 'out :: ToDPDKDevice(0)'
print

# arp. Pattern 0 (port 0) is IP packets. Pattern 1 (port 1) is ARP replies.
# Pattern 2 (port 2) is ARP requests.
print 'arp_c :: Classifier(12/0800, 12/0806 20/0002, 12/0806 20/0001)'
print 'arp_q :: ARPQuerier($DEVNAME:ip, $DEVNAME:eth)'
print 'arp_r :: ARPResponder($DEVNAME)'
print

# defining input classifiers (i.e., packets from vhost h13 (rack 1, host 3) go
# to switch input port 1, packets from h24 go to switch input port 2, etc.)
print 'elementclass in_classify {'
print '  input[0] -> IPClassifier('
for i in xrange(1, NUM_RACKS+1):
    rack_str = '    src host $IP%d or ' % (i)
    k = 1
    for j in xrange(1, HOSTS_PER_RACK+1):
        rack_str += 'src host $IP%d%d or ' % (i, j)
        k += 1
        if k == 4:
            rack_str = rack_str[:-1]
            rack_str += '\n    '
    if i < NUM_RACKS:
        print rack_str[:-4] + ','
    else:
        print rack_str[:-4]
print '  )'
print '  => %soutput' % (str(list(xrange(NUM_RACKS))))
print '}'
print

# defining out classifiers (i.e., packets to vhost h13 (rack 1, host 3) go
# to switch output port 1, packets to h24 go to switch output port 2, etc.)
print 'elementclass out_classify {'
print '  input[0] -> IPClassifier('
for i in xrange(1, NUM_RACKS+1):
    rack_str = '    dst host $IP%d or ' % (i)
    k = 1
    for j in xrange(1, HOSTS_PER_RACK+1):
        rack_str += 'dst host $IP%d%d or ' % (i, j)
        k += 1
        if k == 4:
            rack_str = rack_str[:-1]
            rack_str += '\n    '
    if i < NUM_RACKS:
        print rack_str[:-4] + ','
    else:
        print rack_str[:-4]
print '  )'
print '  => %soutput' % (str(list(xrange(NUM_RACKS))))
print '}'
print

# defining a packet link
print 'elementclass packet_link {'
print '  input%s' % (str(list(xrange(NUM_RACKS))))
print '    => RoundRobinSched'
print '    -> lu :: LinkUnqueue($PACKET_LATENCY_s_TDF, $PACKET_BW_Gbps_TDF)'
print '    -> output'
print '}'
print

# defining a circuit link
print 'elementclass circuit_link {'
print '  input%s' % (str(list(xrange(NUM_RACKS))))
print '    => ps :: SimplePullSwitch(-1)'
print '    -> lu :: LinkUnqueue($CIRCUIT_LATENCY_s_TDF, $CIRCUIT_BW_Gbps_TDF)'
print '    -> StoreData(1, 1) -> SetIPChecksum'  # did packet go over circuit?
print '    -> output'
print '}'
print


##########################
# Packet Switch Definition
##########################
print 'elementclass packet_switch {'

# out classifier definition (used to put packets into right VOQ)
out_classify = '  '
for i in xrange(1, NUM_RACKS+1):
    out_classify += 'c%d, ' % i
print out_classify[:-2] + ' :: out_classify'
print

# VOQ definitions
for i in xrange(1, NUM_RACKS+1):
    queues = '  '
    for j in xrange(1, NUM_RACKS+1):
        queues += 'q%d%d, ' % (i, j)
    queues = queues[:-1]
    if i < NUM_RACKS:
        print queues
    else:
        print queues[:-1]
print ' :: Queue(CAPACITY 3)'
print

# packet down links definitions
pl = ''
k = 0
for i in xrange(1, NUM_RACKS+1):
    pl += 'packet_link%d, ' % i
    k += 1
    if k == 4:
        pl = pl[:-1]
        pl += '\n  '
        k = 0
pl = pl.strip()[:-1]
pl = '  ' + pl + ' :: packet_link'
print pl
print

# wiring input to VOQs
for i in xrange(1, NUM_RACKS+1):
    input = '  input[%d] -> c%d => ' % (i-1, i)
    for j in xrange(1, NUM_RACKS+1):
        input += 'q%d%d, ' % (i, j)
    input = input[:-2]
    print input
print

# wiring VOQs to packet down links to outputs
for i in xrange(1, NUM_RACKS+1):
    output = '  '
    for j in xrange(1, NUM_RACKS+1):
        output += 'q%d%d, ' % (j, i)
    output = output[:-2]
    output += ' => packet_link%d -> [%d]output' % (i, i-1)
    print output
print

# if packets would be dropped at queue, send them out
# a special output for reuse (see hybrid switch)
for i in xrange(1, NUM_RACKS+1):
    output = '  '
    for j in xrange(1, NUM_RACKS+1):
        output += 'q%d%d[1], ' % (i, j)
    output = output[:-2]
    output += ' -> [%d]output' % (i-1 + NUM_RACKS)
    print output

print '}'
print
##############################
# End Packet Switch Definition
##############################


###############
# Hybrid Switch
###############
print 'hybrid_switch :: {'

# out classifier definition (used to put packets into right VOQ)
out_classify = '  '
for i in xrange(1, NUM_RACKS+1):
    out_classify += 'c%d, ' % i
print out_classify[:-2] + ' :: out_classify'
print

# VOQ definitions
for i in xrange(1, NUM_RACKS+1):
    queues = '  '
    for j in xrange(1, NUM_RACKS+1):
        queues += 'q%d%d, ' % (i, j)
    queues = queues[:-1]
    if i < NUM_RACKS:
        print queues
    else:
        print queues[:-1]
print ' :: {'
# We need to name the LockQueue "q" so that EstimateTraffic can find it. If
# threshold-based ECN marking is enabled (e.g., for DCTCP), then LockQueue
# annotates a packet with whether it should be marked.
print '      input[0] -> q :: LockQueue(CAPACITY $SMALL_BUFFER_SIZE)'
# lq is the loss queue. It fills up with packets that were dropped in the packet
# switch. lq packets will have priority over q packets (see PrioSched, below).
print '      input[1] -> lq :: Queue(CAPACITY 5)'
# lq and q connect to input ports 0 and 1, respectively. When trying to pull a
# packet, PrioSched checks its input ports in order, starting from port 0,
# meaning that packets from port 0 (lq) will always be pulled before packets
# from port 1 (q).
print '      lq, q => PrioSched -> output'
print ' }'
print

# circuit link definition
cl = ''
k = 0
for i in xrange(1, NUM_RACKS+1):
    cl += 'circuit_link%d, ' % i
    k += 1
    if k == 4:
        cl = cl[:-1]
        cl += '\n  '
        k = 0
cl = cl.strip()[:-1]
cl = '  ' + cl + ' :: circuit_link'
print cl
print

# packet up link definition
pl = ''
k = 0
for i in xrange(1, NUM_RACKS+1):
    pl += 'packet_up_link%d, ' % i
    k += 1
    if k == 4:
        pl = pl[:-1]
        pl += '\n  '
        k = 0
pl = pl.strip()[:-1]
pl = '  ' + pl + ' :: packet_link'
print pl
print

# packet switch definition
print '  ps :: packet_switch'
print

# wiring inputs to VOQs
for i in xrange(1, NUM_RACKS+1):
    # Input port paint
    input = '  input[%d] -> Paint(%d, 20) -> c%d => ' % (i-1, i, i)
    for j in xrange(1, NUM_RACKS+1):
        input += 'q%d%d, ' % (i, j)
    input = input[:-2]
    print input
print

# wiring VOQs to circuit links to outputs
for i in xrange(1, NUM_RACKS+1):
    output = '  '
    for j in xrange(1, NUM_RACKS+1):
        output += 'q%d%d, ' % (j, i)
    output = output[:-2]
    # dest paint
    output += ' => circuit_link%d -> Paint(%d, 21) -> ' \
              '[%d]output' % (i, i, i-1)
    print output
print

# wiring VOQs to an on/off element to allow packet switch shutoff
# for individual VOQs (i.e., when the circuit link is on)
for i in xrange(1, NUM_RACKS+1):
    for j in xrange(1, NUM_RACKS+1):
        print '  q%d%d -> pps%d%d :: SimplePullSwitch(0)' % (i, j, i, j)

# wiring on/off elements to packet up link to packet switch to output
for i in xrange(1, NUM_RACKS+1):
    output = '  '
    for j in xrange(1, NUM_RACKS+1):
        output += 'pps%d%d, ' % (i, j)
    output = output[:-2]
    # dest paint
    output += ' => packet_up_link%d -> [%d]ps[%d] -> ' \
              'Paint(%d, 21) -> [%d]output' % (i, i-1, i-1, i, i-1)
    print output
print

# Wiring dropped packet switch packets to loss queues in respective VOQ.
for i in xrange(1, NUM_RACKS+1):
    output = '  ps[%d] -> out_classify => ' % (i-1 + NUM_RACKS)
    for j in xrange(1, NUM_RACKS+1):
        output += '[1]q%d%d, ' % (i, j)
    output = output[:-2]
    print output

print '}'
print
###################
# End Hybrid Switch
###################

##################
# Main Connections
##################
print 'in -> arp_c'
print '   -> MarkIPHeader(14)'
print '   -> StripToNetworkHeader '
print '   -> GetIPAddress(IP dst)'
# The second pattern matches all packets, i.e., any packets that do not match
# the first pattern. Packets that match the second pattern are forwarded out
# port 1, which is connected to the downstream elements.
print '   -> pc :: IPClassifier(dst host $DEVNAME:ip icmp echo, -)[1] '

# Only diverts ACKs if set to 1 (meaning that packets should be sent our port
# 1). Used in validation. Normally, acks are passed through to port 0 (set by
# the "0" parameter). When connecting elements together, since we do not specify
# a port to connect to the next element, by default port 0 is used.
print '   -> divert_acks :: Switch(0)'

# Set the time at which the packet hits this element.
print '   -> st :: SetTimestamp(FIRST true) '
# Split the packet stream based on rack.
print '   -> in_classify%s' % (str(list(xrange(NUM_RACKS))))
# The hybrid switch itself.
print '   => hybrid_switch%s' % (str(list(xrange(NUM_RACKS))))

# Packet logging. The name is required so that RunSchedule can call its
# handlers.
print '   -> hsl :: HSLog($NUM_RACKS)'
# ECE marking (for reTCP). The name is required so that RunSchedule can call
# its handler.
print '   -> ecem :: ECEMark($NUM_RACKS)'
# Packets then pass through MarkIPCE, which looks at the THRESH_EXCEEDED_ANNO
# user annotation and, if it equals 1, sets the packet's ECN bits to "Congestion
# Experienced". This element must be named so that scripts can call its write
# handlers.
print '   -> ecn :: MarkIPCE'
print '   -> arp_q '
print '   -> out'
print

# Used in validation. Port 1 is connected to a different path. Packets normally
# go out port 0. Setting the element's "switch" handler to 1 forwards packets
# out port 1 instead.
print 'divert_acks[1] ' \
    '-> acks :: IPClassifier(tcp ack and len < 100, -)[1] -> st'
print 'acks -> arp_q'
print

# Connect ARP replies to the ARPQuerier.
print 'arp_c[1] -> [1]arp_q'
# Connect ARP requests to the ARPResponder.
print 'arp_c[2] -> arp_r -> out'
print

# ping responder. pc[0] is ICMP echo packets.
print 'pc -> ICMPPingResponder -> arp_q'
######################
# End Main Connections
######################
