#!/usr/bin/env python

import collections
import glob
import multiprocessing
from os import path
import shelve
import sys
# Directory containing this program.
PROGDIR = path.dirname(path.realpath(__file__))
# For parse_logs.
sys.path.insert(0, path.join(PROGDIR, "..", ".."))
# For python_config.
sys.path.insert(0, path.join(PROGDIR, "..", "..", "..", "etc"))

import dotmap
import simpleplotlib
simpleplotlib.default_options.rcParams["font.family"] = "Tahoma"

import parse_logs
import python_config

# True and False mean that the data parsing will be executed using a single
# thread and multiple threads, respectively.
SYNC = False
# Maps experiment to filename.
FILES = {
    "static": "*-fixed-*-False-*-reno-*click.txt",
    "resize": "*-QUEUE-True-*-reno-*click.txt",
}
# Maps experiment to a function that convert a filename to an integer key
# identifying this experiment (i.e., for the legend).
KEY_FN = {
    "static": lambda fn: int(fn.split("fixed-")[1].split("-")[0]),
    "resize": lambda fn: int(fn.split("True-")[1].split("-")[0]) / python_config.TDF,
}
# Kilo-sequence number
UNITS = 1000.0
# Control which chunk to select.
CHUNKS_BEST = True
FLOW = 0
CHUNK = 10


class FileReader(object):
    def __init__(self, name):
        self.name = name

    def __call__(self, fn):
        return (KEY_FN[self.name](fn.split("/")[-1]),
                parse_logs.get_seq_data(fn))


def add_optimal(data):
    """
    Adds the calculated baselines (optimal and packet-only) to the provided data
    dictionary.
    """
    # Compute optimal sequence numbers. Recall that sequence numbers are in
    # terms of bytes, so we use bytes in some of the below comments.
    print("Computing optimal...")

    # Calculate the raw rate of the packet and circuit networks.
    #
    # Constant used to convert Gb/s to the units of the graphs. E.g., using
    # HOSTS_PER_RACK = 16 and UNITS = KB, our target is KB / us / host:
    #
    #                                 div by    div by
    #                           HOSTS_PER_RACK  UNITS
    #
    #     10**9 b    1 B      1 s     1 rack     1 KB    0.0078125 KB
    #     ------- x  --- x -------- x ------- x ------ = ------------
    #       Gb       8 b   10**6 us   16 host   1000 B    us x host
    factor = 10**9 / 8. / 10**6 / python_config.HOSTS_PER_RACK / UNITS
    pr_KBpus = python_config.PACKET_BW_Gbps * factor
    cr_KBpus = python_config.CIRCUIT_BW_Gbps * factor

    # Circuit start and end times, of the form:
    #     [<start>, <end>, <start>, <end>, ...]
    bounds = [int(round(q)) for q in data["lines"]]
    assert len(bounds) % 2 == 0, \
        ("Circuit starts and ends must come in pairs, but the list of them "
         "contains an odd number of elements: {}".format(bounds))
    print("circuit bounds: {}".format(bounds))

    # Each entry is the maximum amount of data that could have been sent by that
    # time.
    optimal = []
    for state in xrange(0, len(bounds), 2):
        # Circuit night.
        if state == 0:
            # 0 to first day start.
            optimal = [pr_KBpus * us for us in xrange(bounds[state])]
        else:
            # Previous day end to current day start.
            optimal += [
                optimal[-1] + pr_KBpus * us
                for us in xrange(1, bounds[state] - bounds[state - 1] + 1)]
        # Current day start to current day end.
        optimal += [
            optimal[-1] + cr_KBpus * us
            for us in xrange(1, bounds[state + 1] - bounds[state] + 1)]

    # Bytes sent if we only used the packet network.
    pkt_only = [pr_KBpus * us for us in xrange(0, bounds[-1])]

    # Verify that in optimal, no two adjacent elements are equal.
    for i in xrange(bounds[-1] - 1):
        assert pkt_only[i] != pkt_only[i + 1], \
            "pkt_only[{}] == pkt_only[{}] == {}".format(
                i, i + 1, pkt_only[i])
        assert optimal[i] != optimal[i + 1], \
            "optimal[{}] == optimal[{}] == {}".format(i, i + 1, optimal[i])

    data["keys"].insert(0, "packet only")
    data["data"].insert(0, pkt_only)
    data["keys"].insert(0, "optimal")
    data["data"].insert(0, optimal)


def get_data(db, key):
    """
    (Optionally) loads the results for the specified key into the provided
    database and returns them.
    """
    if key not in db:
        ptns = FILES[key]
        if not isinstance(ptns, list):
            ptns = [ptns]
        # For each pattern, extract the matches. Then, flatten them into a
        # single list.
        fns = [fn for matches in
               [glob.glob(path.join(sys.argv[1], ptn)) for ptn in ptns]
               for fn in matches]

        assert fns, "Found no files for patterns: {}".format(ptns)
        print("Found files for patterns: {}".format(ptns))
        for fn in fns:
            print("    {}".format(fn))

        data = collections.defaultdict(dict)

        if SYNC:
            data["raw_data"] = dict([FileReader(key)(fn) for fn in fns])
        else:
            p = multiprocessing.Pool()
            data["raw_data"] = dict(p.map(FileReader(key), fns))
            # Clean up p.
            p.close()
            p.join()

        data["raw_data"] = sorted(data["raw_data"].items())
        data["keys"] = list(zip(*data["raw_data"])[0])
        data["lines"] = data["raw_data"][0][1][1]
        data["data"] = [[y / UNITS for y in f]
                        for f in zip(*zip(*data["raw_data"])[1])[0]]

        # Convert the results for each set of original chunk data.
        data["chunks_orig"] = {}
        # Look through each line.
        for line, (_, _, chunks_orig_all) in data["raw_data"]:
            data["chunks_orig"][line] = {}
            # Look through each flow in this line.
            for flw, chunks_orig in chunks_orig_all.items():
                data["chunks_orig"][line][flw] = []
                # Look through each chunk in this flow.
                for chunk_orig in chunks_orig:
                    xs, ys = chunk_orig
                    data["chunks_orig"][line][flw].append(
                        (xs, [y / UNITS for y in ys]))

        # Select the best chunk for each line.
        data["chunks_best"] = {}
        # Look through each line.
        for line, chunks_orig_all in data["chunks_orig"].items():
            data["chunks_best"][line] = ([], [])
            # Look through flow in this line.
            for chunks_orig in chunks_orig_all.values():
                # Look through each chunk in this flow.
                for chunk_orig in chunks_orig:
                    if len(chunk_orig[0]) > len(data["chunks_best"][line][0]):
                        data["chunks_best"][line] = chunk_orig

        add_optimal(data)
        # Store the new data in the database.
        db[key] = data

    chunks_selected_key = "chunks_selected_flow{}_chunk{}".format(FLOW, CHUNK)
    # Do not factor database items into temporary variables to eleminate
    # unnecesary data copies.
    if chunks_selected_key not in db[key]:
        # Select a particular chunk for each line. Store the results in the
        # database under a unique key so that we can change the selected flow
        # and chunk without reparsing the data.
        db[key][chunks_selected_key] = {}
        # Look through each line.
        for line in db[key]["chunks_orig"].keys():
            flow_key = db[key]["chunks_orig"][line].keys()[FLOW]
            db[key][chunks_selected_key][line] = \
                db[key]["chunks_orig"][line][flow_key][CHUNK]
    return db[key]


def plot_seq(data, fn, odr=path.join(PROGDIR, "..", "graphs"),
             ins=None, flt=lambda idx, label: True, order=None, xlm=None,
             ylm=None, chunk_mode=False):

    # Select the data based on whether we are plotting a single chunk from a
    # single flow or aggregate metrics for all chunks from all flows.
    if chunk_mode:
        key = "chunks_best" if CHUNKS_BEST else \
            "chunks_selected_flow{}_chunk{}".format(FLOW, CHUNK)
        xs, ys = zip(*data[key].values())
    else:
        ys = data["data"]
        xs = [xrange(len(ys[i])) for i in xrange(len(ys))]

    # Format the legend labels.
    lls = []
    for k in data["keys"]:
        try:
            if "static" in fn:
                lls += ["%s packets" % int(k)]
            else:
                lls += ["%s $\mu$s" % int(k)]
        except ValueError:
            lls += [k]

    options = dotmap.DotMap()
    options.plot_type = "SCATTER" if chunk_mode else "LINE"
    options.legend.options.loc = "center right"
    options.legend.options.bbox_to_anchor = (1.4, 0.5)
    options.legend.options.labels = lls
    options.legend.options.fontsize = 18
    # Use 1 column if there are fewer than 4 lines, otherwise use 2 columns.
    options.legend.options.ncol = 1  # if len(data["data"]) < 4 else 2
    options.series_options = [
        dotmap.DotMap(linewidth=2) for i in range(len(xs))]
    options.output_fn = path.join(odr, "{}.pdf".format(fn))
    if xlm is not None:
        options.x.limits = xlm
    if ylm is not None:
        options.y.limits = ylm
    options.x.label.xlabel = "Time ($\mu$s)"
    options.y.label.ylabel = "Expected TCP sequence\nnumber ($\\times$1000)"
    options.x.label.fontsize = options.y.label.fontsize = 18
    options.x.ticks.major.options.labelsize = \
        options.y.ticks.major.options.labelsize = 18
    options.x.axis.show = options.y.axis.show = True
    options.x.axis.color = options.y.axis.color = "black"
    lines = data["lines"]
    options.vertical_lines.lines = lines
    shaded = []
    for i in xrange(0, len(lines), 2):
        shaded.append((lines[i], lines[i+1]))
    options.vertical_shaded.limits = shaded
    options.vertical_shaded.options.alpha = 0.1
    options.vertical_shaded.options.color = "blue"

    if ins is not None:
        options.inset.show = True
        options.inset.options.zoom_level = 1.75
        options.inset.options.corners = (2, 3)
        options.inset.options.location = "center right"
        options.inset.options.marker.options.color = "black"
        xlm_ins, ylm_ins = ins
        options.inset.options.x.limits = xlm_ins
        options.inset.options.y.limits = ylm_ins

    # Pick only the lines that we want.
    if flt is not None:
        xs, ys, options.legend.options.labels = zip(
            *[(x, y, l) for (i, (x, y, l)) in enumerate(
                zip(xs, ys, options.legend.options.labels))
              if flt(i, l)])

    if order is not None:
        real_xs = []
        real_ys = []
        real_ls = []
        for item in order:
            idx = 0
            found = False
            for possibility in options.legend.options.labels:
                if possibility.startswith(item):
                    found = True
                    break
                idx += 1
            if found:
                real_xs.append(xs[idx])
                real_ys.append(ys[idx])
                real_ls.append(options.legend.options.labels[idx])
        xs = real_xs
        ys = real_ys
        options.legend.options.labels = real_ls

    simpleplotlib.plot(xs, ys, options)


def rst_glb(dur):
    """ Reset global variables. """
    global FILES, KEY_FN
    # Reset global lookup tables.
    FILES = {}
    KEY_FN = {}
    # Reset experiment duration.
    parse_logs.DURATION = dur
    # Do not set sg.DURATION because it get configured automatically based on
    # the actual circuit timings.


def seq(name, edr, odr, ptn, key_fnc, dur, ins=None, flt=None, order=None,
        xlm=None, ylm=None, chunk_mode=False):
    """ Create a sequence graph.

    name: Name of this experiment, which become the output filename.
    edr: Experiment dir.
    odr: Output dir.
    ptn: Glob pattern for experiment files.
    key_fnc: Function that takes an experiment data filename returns a legend
             key.
    dur: The duration of the experiment, in milliseconds.
    ins: An inset specification.
    flt: Function that takes a legend index and label and returns a boolean
         indicating whether to include that line.
    order: List of the legend labels in their desired order.
    xlm: x-axis limits
    ylm: y-axis limits
    """
    print("Plotting: {}".format(name))
    rst_glb(dur)
    # Names are of the form "<number>_<details>_<specific options>". Experiments
    # where <details> are the same should be based on the same data. Therefore,
    # use <details> as the database key.
    if "_" in name:
        basename = name.split("_")[1]
    else:
        basename = name
    FILES[basename] = ptn
    KEY_FN[basename] = key_fnc
    db = shelve.open(path.join(edr, "{}.db".format(basename)))
    plot_seq(get_data(db, basename), name, odr, ins, flt, order, xlm, ylm,
             chunk_mode)
    db.close()
