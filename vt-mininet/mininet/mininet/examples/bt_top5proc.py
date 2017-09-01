import sys
from collections import Counter
import babeltrace


def top5proc():
    if len(sys.argv) != 2:
        msg = 'Usage: python {} TRACEPATH'.format(sys.argv[0])
        raise ValueError(msg)

    # a trace collection holds one to many traces
    col = babeltrace.TraceCollection()

    # add the trace provided by the user
    # (LTTng traces always have the 'ctf' format)
    if col.add_trace(sys.argv[1], 'ctf') is None:
        raise RuntimeError('Cannot add trace')

    # this counter dict will hold execution times:
    #
    #   task command name -> total execution time (ns)
    exec_times = Counter()

    # this holds the last `sched_switch` timestamp
    last_ts = None

    # iterate events
    for event in col.events:
        # keep only `sched_switch` events
        if event.name != 'sched_switch':
            continue

        # keep only events which happened on CPU 0
        if event['cpu_id'] != 0:
            continue

        # event timestamp
        cur_ts = event.timestamp

        if last_ts is None:
            # we start here
            last_ts = cur_ts

        # previous task command (short) name
        prev_comm = event['prev_comm']

        # initialize entry in our dict if not yet done
        if prev_comm not in exec_times:
            exec_times[prev_comm] = 0

        # compute previous command execution time
        diff = cur_ts - last_ts

        # update execution time of this command
        exec_times[prev_comm] += diff

        # update last timestamp
        last_ts = cur_ts

    # display top 10
    for name, ns in exec_times.most_common(5):
        s = ns / 1000000000
        print('{:20}{} s'.format(name, s))


if __name__ == '__main__':
    top5proc()

    