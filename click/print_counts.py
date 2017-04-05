#!/usr/bin/env python

import socket
import sys
import time

n = 4

TCP_IP = 'localhost'
TCP_PORT = 1239
BUFFER_SIZE = 1024

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.connect((TCP_IP, TCP_PORT))
data = s.recv(BUFFER_SIZE)

while(True):
    results = []
    for i in range(n):
        results.append([])
        for j in range(n):
            message = 'READ out%s/c%s.bit_rate\n' % (j, i)
            s.send(message)
            data = s.recv(BUFFER_SIZE)
            if data[:3] == '200':
                results[i].append(int(data.split('\n')[-1]) / 1000000)
                # results[i].append(int(round(float(data.split('\n')[-1])*8 /
                #                             1000000)))
    # message = 'READ c0.bit_rate\n'
    # s.send(message)
    # data = s.recv(BUFFER_SIZE)
    # if data[:3] == '200':
    #     results.append(int(data.split('\n')[-1]) / 1000000)
    print '\r%s' % results,
    sys.stdout.flush()
    time.sleep(0.1)
s.close()
