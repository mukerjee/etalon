#!/usr/bin/env python

import mmap
import os
from os import path
import sys
import time


def parse_adr(adr):
    """
    Converts an IP address + port in the form aaaaaaaa:pppp to the form
    a.b.c.d:p.
    """
    ip_raw, port_raw = adr.split(":")
    # Split into a list of the octets.
    ip_hexs = [ip_raw[idx: idx + 2] for idx in xrange(0, len(ip_raw), 2)]
    # Convert each octet to an int.
    ip_ints = [int(hex_str, 16) for hex_str in ip_hexs]
    ip_ints.reverse()
    return("{}:{}".format(".".join([str(x) for x in ip_ints]), int(port_raw, 16)))


def main():
    assert len(sys.argv) == 6, \
        ("Expected five arguments: logging interval (seconds), logging "
         "duration (seconds), local IP address + port (a.b.c.d:p1), remote IP "
         "address + port (e.f.g.h:p2), output file")
    intv_s, dur_s, lcl_adr_tgt, rem_adr_tgt, out = sys.argv[1:]
    dur_s = float(dur_s)
    intv_s = float(intv_s)
    # Make sure that the output file does not already exist.
    if path.exists(out):
        print("Output file already exists: {}".format(out))
        sys.exit(-1)
    else:
        # Create the output directory if it does not already exist.
        odr = path.dirname(out)
        if odr and not path.isdir(odr):
            os.makedirs(odr)

    # While polling the file, only record the lines to maximize sample rate.
    start_s = time.time()
    cur_s = start_s
    delt_s = 0
    tstamp_lines = []
    with open("/proc/net/tcp", "r+") as f:
        while (delt_s < dur_s):
            delt_s = time.time() - start_s
            tstamp_lines.append((delt_s, [line for line in f]))
            f.seek(0)
            time.sleep(intv_s)

    # Do all the data parsing once we are done..
    cwnds = []
    for tstamp_s, lines in tstamp_lines:
        # Find the lines corresponding to outgoing connections, and extract
        # their cwnds. Skip the first line, which is the column headings.
        for line in lines[1:]:
            splits = line.strip().split()
            lcl_adr = parse_adr(splits[1])
            rem_adr = parse_adr(splits[2])
            if lcl_adr == lcl_adr_tgt and rem_adr == rem_adr_tgt:
                cwnds.append((tstamp_s, int(splits[15])))

    with open(out, "w") as f:
        for tstamp_s, cwnd in cwnds:
            f.write("{},{}\n".format(tstamp_s, cwnd))


if __name__ == "__main__":
    main()
