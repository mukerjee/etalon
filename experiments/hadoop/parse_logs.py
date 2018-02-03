#!/usr/bin/env python

import sys
import glob
import numpy as np

TDF = 20.0


def parse_hadoop_logs(folder):
    data = []
    durations = []

    fn = folder + '/*-logs/hadoop*-datanode*.log'
    print fn
    logs = glob.glob(fn)

    for log in logs:
        for line in open(log):
            if 'clienttrace' in line and 'HDFS_WRITE' in line:
                bytes = int(line.split('bytes: ')[1].split()[0][:-1])
                if bytes > 1024 * 1024 * 99:
                    duration = (float(line.split(
                        "duration: ")[1].split()[0]) * 1e-6) / TDF
                    data.append((bytes / 1024. / 1024., duration))

    durations = sorted(zip(*data)[1])
    # print durations
    print np.percentile(durations, 50), np.percentile(durations, 99)
    return durations


def parse_total_writes(folder):
    fn = folder + '/report/dfsioe/hadoop/bench.log'
    logs = glob.glob(fn)

    for log in logs:
        for line in open(log):
            if 'Total time spent by all map tasks' in line:
                return (float(line.split('(ms)=')[1]) * 1e-3) / TDF


def parse_job_completion_time(folder):
    fn = folder + '/report/dfsioe/hadoop/bench.log'
    logs = glob.glob(fn)

    for log in logs:
        for line in open(log):
            if 'Test exec time sec:' in line:
                return (float(line.split('sec: ')[1])) / TDF


def parse_throughput(folder):
    fn = folder + '/report/dfsioe/hadoop/bench.log'
    logs = glob.glob(fn)

    for log in logs:
        for line in open(log):
            if 'Number of files' in line:
                num_files = int(line.split('files: ')[1])
            if 'Throughput mb/sec:' in line:
                return (float(line.split('mb/sec:')[1]) * num_files * 8 / 1024.0) * TDF

if __name__ == "__main__":
    parse_hadoop_logs(sys.argv[1])
