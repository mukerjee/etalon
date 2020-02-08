#!/usr/bin/env python

import collections
from os import path
import sys
# Directory containing this program.
PROGDIR = path.dirname(path.realpath(__file__))
# For parse_logs.
sys.path.insert(0, path.join(PROGDIR, ".."))
# For python_config.
sys.path.insert(0, path.join(PROGDIR, "..", "..", "etc"))
import shelve
import glob
import numpy as np

import dotmap
from matplotlib import pyplot
import simpleplotlib
simpleplotlib.default_options.rcParams["font.family"] = "Tahoma"

import parse_logs
import python_config

SR_RACKS = (1, 2)


def get_data(rdb_filepath, edr, key, files, key_fnc, msg_len=112):
    # Open results database file.
    rdb = shelve.open(rdb_filepath, protocol=2, writeback=True)
    data = rdb.get(key)

    if data is None:
        ptn = path.join(edr, files[key])
        flns = glob.glob(ptn)
        assert flns, "Found no files for pattern: {}".format(ptn)
        print("Found files for pattern: {}".format(ptn))
        for fln in flns:
            print("    {}".format(fln))

        data = collections.defaultdict(lambda: collections.defaultdict(dict))
        for fln in flns:
            lbl = key_fnc[key](path.basename(fln))
            lats, (_, tpt_c, _), _, _ = parse_logs.parse_packet_log(
                fln, msg_len)
            data["tpt_c"][lbl] = tpt_c[SR_RACKS]
            # First, convert from grouping based on combined, circuit, and
            # packet latency to grouping backed on percentile. Then, extract the
            # values for a certain percentile. Finally, drop the percentile.
            data["lat"][50][lbl] = [
                lt for _, lt in zip(*lats)[parse_logs.PERCENTILES.index(50)]]
            data["lat"][99][lbl] = [
                lt for _, lt in zip(*lats)[parse_logs.PERCENTILES.index(99)]]

        # Convert from dictionary to key-value pairs, then sort by the key, then
        # extract the keys only.
        data["keys"] = list(zip(*sorted(data["tpt_c"].items()))[0])
        # Convert from dictionary to key-value pairs, then sort by the key, then
        # extract the values only.
        data["lat"][50] = list(zip(*sorted(data["lat"][50].items()))[1])
        data["lat"][99] = list(zip(*sorted(data["lat"][99].items()))[1])
        data["tpt_c"] = list(zip(*sorted(data["tpt_c"].items()))[1])
        # Store the new data in the database.
        rdb[key] = dict(data)

    rdb.close()
    return data


def plot_lat(keys, latencies, fln, ylb, ylm=None, xlr=0,
             odr=path.join(PROGDIR, "graphs")):
    # Sort the data based on the x-values (keys).
    keys, latencies = zip(
        *sorted(zip(keys, latencies), key=lambda p: int(p[0])))

    x = [keys for _ in xrange(len(latencies[0]))]
    y = zip(*latencies)

    print("")
    print("raw latency data for: {}".format(fln))
    print("{}:".format(ylb.strip("\n")))
    print("    all: {}".format(
        ", ".join(["({}: {})".format(a, b) for a, b in zip(x[0], y[0])])))
    print("    circuit: {}".format(
        ". ".join(["({}: {})".format(a, b) for a, b in zip(x[1], y[1])])))
    print("    packet: {}".format(
        ", ".join(["({}: {})".format(a, b) for a, b in zip(x[2], y[2])])))
    print("")

    options = dotmap.DotMap()
    options.legend.options.labels = ["all traffic", "only circuit",
                                     "only packet"]
    options.legend.options.fontsize = 20
    options.output_fn = path.join(odr, "{}.pdf".format(fln))
    options.plot_type = "LINE"
    options.series_options = [
        dotmap.DotMap(marker="o", markersize=10, linewidth=5)
        for _ in xrange(len(x))]
    options.x.label.fontsize = options.y.label.fontsize = 20
    options.x.label.xlabel = "Buffer size (packets)" if "static" in fln \
        else "Early buffer resizing ($\mu$s)"
    options.x.ticks.major.options.labelsize = \
        options.y.ticks.major.options.labelsize = 20
    options.x.ticks.major.labels = \
        dotmap.DotMap(locations=[4, 8, 16, 32, 64, 128]) \
        if "static" in fln else dotmap.DotMap(locations=keys)
    options.x.ticks.major.labels.options.rotation = xlr
    options.x.ticks.major.labels.options.rotation_mode = "anchor"
    options.x.ticks.major.labels.options.horizontalalignment = \
        "center" if xlr == 0 else "right"
    options.y.label.ylabel = "{} latency ($\mu$s)".format(ylb)
    if ylm is not None:
        options.y.limits = [0, ylm]
    simpleplotlib.plot(x, y, options)


def plot_circuit_util(keys, tpts_Gbps_c, fln, xlb, num_racks,
                      odr=path.join(PROGDIR, "graphs"), srt=True, xlr=0, lbs=23,
                      flt=lambda key: True):
    """ srt: sort, xlr: x label rotation (degrees), lbs: bar label size """
    if srt:
        # Sort the data based on the x-values (keys).
        keys, tpts_Gbps_c = zip(*sorted(zip(keys, tpts_Gbps_c)))
    # Filter.
    keys, tpts_Gbps_c = zip(
        *[(key, tpt_Gbps_c)
          for key, tpt_Gbps_c in zip(keys, tpts_Gbps_c) if flt(key)])
    # Convert circuit throughput into circuit utilization. The throughput values
    # are averages over the entire experiment, including when the circuit
    # network was not active: they were computed by dividing the number of bytes
    # transported by the circuit network over the course the entire experiment
    # by the length of the experiment. To calculate the average circuit
    # utilization, we divide the achieved average throughput by the maximum
    # possible average throughput. The maximum possible average throughput is
    # equal to the circuit bandwidth multiplied by the fraction of the
    # experiment during which the circuit network was in use. This is equal to
    # the fraction of the schedule during which a circuit was assigned between
    # the racks in question (1 / (num_racks - 1)) times the fraction of that
    # time during which the circuit was up (reconfiguration time / day length =
    # 0.9). Therefore:
    #
    # util =            throughput           = throughput * (num_racks - 1)
    #        -------------------------------   ----------------------------
    #        bandwidth *       1       * 0.9         0.9 * bandwidth
    #                    -------------
    #                    num_racks - 1
    #
    # Finally, we multiply by 100 to convert to a percent.
    utls = [tpt_Gbps_c * (num_racks - 1) /
            (0.9 * python_config.CIRCUIT_BW_Gbps) * 100
            for tpt_Gbps_c in tpts_Gbps_c]

    print("")
    print("raw util data for: {}".format(fln))
    print("{}:".format(xlb))
    print("    {}".format(", ".join(
        ["({}: {})".format(key, utl) for key, utl in zip(keys, utls)])))
    print("")

    options = dotmap.DotMap()
    options.plot_type = "BAR"
    options.legend.options.fontsize = 20
    options.bar_labels.format_string = "%1.0f"
    options.bar_labels.options.fontsize = lbs
    options.output_fn = path.join(odr, "{}.pdf".format(fln))
    options.y.limits = (0, 100)
    options.x.label.fontsize = options.y.label.fontsize = 20
    options.x.label.xlabel = xlb
    options.y.label.ylabel = "Average circuit\nutilization (%)"
    options.x.ticks.major.options.labelsize = \
        options.y.ticks.major.options.labelsize = 20
    options.x.ticks.major.labels = dotmap.DotMap(text=keys)
    options.x.ticks.major.labels.options.rotation = xlr
    options.x.ticks.major.labels.options.rotation_mode = "anchor"
    options.x.ticks.major.labels.options.horizontalalignment = \
        "center" if xlr == 0 else "right"
    options.y.ticks.major.show = True
    options.x.ticks.major.show = False
    simpleplotlib.plot([np.arange(len(utls))], [utls], options)


def plot_util_vs_latency(tpts, latencies, fln):
    x = [[min(
        j / (0.9 * 1. / (python_config.NUM_RACKS - 1)
             * python_config.CIRCUIT_BW_Gbps) * 100,
        100.0) for j in t]
         for t in tpts]
    y = [zip(*l)[0] for l in latencies]

    options = dotmap.DotMap()
    options.plot_type = "LINE"
    options.legend.options.labels = ["Static buffers (vary size)",
                                     "Dynamic buffers (vary $\\tau$)",
                                     "reTCP",
                                     "reTCP + dynamic buffers (vary $\\tau$)"]
    options.legend.options.fontsize = 19
    options.series_options = [
        dotmap.DotMap(marker="o", markersize=10, linewidth=5)
        for _ in xrange(len(x))]
    options.series_options[2].marker = "x"
    options.series_options[2].s = 100
    del options.series_options[2].markersize
    options.series_options[2].zorder = 10
    options.output_fn = \
        path.join(PROGDIR, "graphs", "throughput_vs_latency99.pdf") \
        if "99" in fln \
           else path.join(PROGDIR, "graphs", "throughput_vs_latency.pdf")
    options.x.label.xlabel = "Circuit utilization (%)"
    options.y.label.ylabel = "99th percent. latency ($\mu$s)" if "99" in fln \
                             else "Median latency ($\mu$s)"
    options.y.limits = [0, 1000] if "99" in fln else [0, 600]
    options.y.ticks.major.labels = \
        dotmap.DotMap(locations=[0, 200, 400, 600, 800, 1000]) \
        if "99" in fln else \
        dotmap.DotMap(locations=[0, 100, 200, 300, 400, 500, 600])

    simpleplotlib.plot(x, y, options)


def lat(name, edr, odr, ptn, key_fnc, prc, ylb, ylm=None, xlr=0, msg_len=112):
    print("Plotting: {}".format(name))
    # Names are of the form "<number>_<details>_<specific options>". Experiments
    # where <details> are the same should be based on the same data. Therefore,
    # use <details> as the database key.
    basename = name.split("_")[1] if "_" in name else name
    data = get_data(path.join(edr, "{}.db".format(basename)), edr, basename,
                    files={basename: ptn}, key_fnc={basename: key_fnc}, msg_len=msg_len)
    plot_lat(data["keys"], data["lat"][prc], name, ylb, ylm, xlr, odr)
    pyplot.close()


def util(name, edr, odr, ptn, key_fnc, xlb, num_racks, srt=True, xlr=0, lbs=23,
         flt=lambda key: True, msg_len=112):
    """
    srt: sort
    xlr: x label rotation (degrees)
    lbs: bar label fontsize
    flt: filter function that takes in a key
    """
    print("Plotting: {}".format(name))
    # Names are of the form "<number>_<details>_<specific options>". Experiments
    # where <details> are the same should be based on the same data. Therefore,
    # use <details> as the database key.
    basename = name.split("_")[1] if "_" in name else name
    data = get_data(path.join(edr, "{}.db".format(basename)), edr, basename,
                    files={basename: ptn}, key_fnc={basename: key_fnc},
                    msg_len=msg_len)
    plot_circuit_util(data["keys"], data["tpt_c"], name, xlb, num_racks, odr,
                      srt, xlr, lbs, flt)
    pyplot.close()
