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
from matplotlib import pyplot
import simpleplotlib
simpleplotlib.default_options.rcParams["font.family"] = "Tahoma"

import parse_logs
import python_config



# Kilo-sequence number
UNITS = 1000.0


class FileReaderArgs(object):
    def __init__(self, dur, key, fln, log_pos="after",
                 msg_len=112):
        self.dur = dur
        self.key = key
        self.fln = fln
        self.log_pos = log_pos
        self.msg_len = msg_len


class FileReader(object):
    def __call__(self, args):
        """
        Parses a single results file. "args" is a FileReaderArgs object. Returns
        tuples of the form (key, results).
        """
        # Results with flow cleaning.
        results, bounds, _ = parse_logs.get_seq_data(
            args.fln, args.dur, args.log_pos, args.msg_len, clean=True)
        # Results without flow cleaning.
        _, _, chunks = parse_logs.get_seq_data(
            args.fln, args.dur, args.log_pos, args.msg_len, clean=False)
        # Take the aggregate results and circuit bounds from the cleaned version
        # and the raw chunk data from the uncleaned version.
        return args.key, (results, bounds, chunks)


def add_optimal(data):
    """
    Adds the calculated baselines (optimal and packet-only) to the provided data
    dictionary. Recall that sequence numbers are in terms of bytes, and that
    during reconfigurations, both the packet and circuit networks are offline.
    """
    print("Computing optimal...")

    # Calculate the raw rate of the packet and circuit networks.
    #
    # "factor" is a constant used to convert Gb/s to the units of the graphs.
    # E.g., using HOSTS_PER_RACK = 16 and UNITS = KB, our target is
    # KB / us / host:
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
    bounds = [int(round(q)) for q in data["circuit_lines"]]
    assert len(bounds) % 2 == 0, \
        ("Circuit starts and ends must come in pairs, but the list of them "
         "contains an odd number of elements: {}".format(bounds))
    print("circuit bounds: {}".format(bounds))

    # Validate the reconfiguration delay.
    rcf_us = python_config.RECONFIG_DELAY_us
    assert rcf_us == round(rcf_us), \
        "The reconfiguration delay must be an integer, but is: {}".format(
            rcf_us)
    rcf_us = int(rcf_us)

    # Each entry is the maximum amount of data that could have been sent up to
    # the beginning of that time unit.
    optimal = []
    for state in xrange(0, len(bounds), 2):
        # Reconfiguration.
        last = optimal[-1] if optimal else 0
        optimal += [last] * rcf_us
        # Circuit night.
        if state == 0:
            # 0 to first day start.
            optimal += [pr_KBpus * us for us in xrange(
                1, bounds[state] - (2 * rcf_us) + 1)]
        else:
            # Previous day end to current day start.
            optimal += [optimal[-1] + pr_KBpus * us for us in xrange(
                1, bounds[state] - bounds[state - 1] - (2 * rcf_us) + 1)]
        # Reconfiguration.
        optimal += [optimal[-1]] * rcf_us
        # Circuit day. Current day start to current day end.
        optimal += [optimal[-1] + cr_KBpus * us for us in xrange(
            1, bounds[state + 1] - bounds[state] + 1)]
    # Bytes sent if we only used the packet network. (Note that, in this case,
    # there are no reconfigurations.)
    pkt_only = [pr_KBpus * us for us in xrange(0, bounds[-1])]

    # Verify that in pkt_only, no two adjacent elements are equal.
    for i in xrange(len(pkt_only) - 1):
        assert pkt_only[i] != pkt_only[i + 1], \
            "pkt_only[{}] == pkt_only[{}] == {}".format(
                i, i + 1, pkt_only[i])

    data["keys"].insert(0, "packet only")
    data["seqs"].insert(0, pkt_only)
    data["keys"].insert(0, "optimal")
    data["seqs"].insert(0, optimal)


def get_data(rdb_filepath, key, ptns, dur, key_fnc, chunk_mode=None,
             log_pos="after", msg_len=112, sync=False):
    """
    (Optionally) loads the results for the specified key into the provided
    database and returns them.
    """
    # Open results database file.
    rdb = shelve.open(rdb_filepath, protocol=2, writeback=True)
    data = rdb.get(key)

    if data is None:
        # For each pattern, extract the matches. Then, flatten them into a
        # single list.
        flns = [fln for matches in
                [glob.glob(path.join(sys.argv[1], ptn)) for ptn in ptns]
                for fln in matches]
        assert flns, "Found no files for patterns: {}".format(ptns)
        print("Found files for patterns: {}\n{}".format(
            ptns, "\n".join(["    {}".format(fln) for fln in flns])))

        args = [
            FileReaderArgs(
                dur, key_fnc(path.basename(fln)), fln, log_pos, msg_len)
            for fln in flns]
        if sync:
            # Single-threaded mode.
            raw_data = [FileReader()(arg) for arg in args]
        else:
            # Multithreaded mode.
            pool = multiprocessing.Pool()
            raw_data = pool.map(FileReader(), args)
            # Clean up pool.
            pool.close()
            pool.join()

        data = collections.defaultdict(dict)
        data["raw_data_"] = sorted(raw_data)

        print("len(data[raw_data]): {}".format(len(data["raw_data"])))
        print("len(data[raw_data][0]): {}".format(len(data["raw_data"][0])))
        print("len(data[raw_data][0][0]): {}".format(len(data["raw_data"][0][0])))

        data["keys"] = list(zip(*data["raw_data"])[0])
        data["circuit_lines"] = data["raw_data"][0][1][1]
        data["seqs"] = [[y / UNITS for y in f]
                        for f in zip(*zip(*data["raw_data"])[1])[0]]

        exit()
        data["voqs"] = zip(*zip(*data["raw_data"])[1])[0]


        
        # Convert the results for each set of original chunk data. Look through
        # each line.
        for line, (_, _, chunks_orig_all) in data["raw_data"]:
            # Check whether all flows have the same number of chunks. Extract
            # the number of chunks per flow.
            num_chunks = {flw: len(chunks_orig)
                          for flw, chunks_orig in chunks_orig_all.items()}
            # Use the number of chunks in the first flow as the target.
            target_num = num_chunks.values()[0]
            all_same = True
            # Check if any flows have a different number of chunks than the
            # target.
            for num in num_chunks.values():
                all_same = all_same and (num == target_num)
            if not all_same:
                print(("Warning: The flows in line \"{}\" have differing "
                       "numbers of chunks:\n  {}").format(line, num_chunks))

            data["chunks_orig"][line] = {}
            # Look through each flow in this line.
            for flw, chunks_orig in chunks_orig_all.items():
                data["chunks_orig"][line][flw] = []
                # Look through each chunk in this flow.
                for chunk_orig in chunks_orig:
                    xs, ys, voq_lens = chunk_orig
                    data["chunks_orig"][line][flw].append(
                        (xs, [y / UNITS for y in ys], voq_lens))

        # Select the best chunk for each line (i.e., the chunk with the most
        # datapoints). Look through each line.
        for line, chunks_orig_all in data["chunks_orig"].items():
            data["chunks_best"][line] = ([], [], [])
            # Look through flow in this line.
            for chunks_orig in chunks_orig_all.values():
                # Look through each chunk in this flow.
                for chunk_orig in chunks_orig:
                    if len(chunk_orig[0]) > len(data["chunks_best"][line][0]):
                        data["chunks_best"][line] = chunk_orig

        # Store the new data in the database.
        rdb[key] = data

    if chunk_mode is not None and chunk_mode != "best":
        # Select a particular chunk for each line. Store the results in the
        # database under a unique key so that we can change the selected chunk
        # without reparsing the data.
        chunks_selected_key = "chunks_selected_chunk{}".format(chunk_mode)
        if chunks_selected_key not in data:
            chunks_selected_data = {}
            # Look through each line.
            for line, chunks_orig_all in data["chunks_orig"].items():
                chunks_selected_data[line] = {}
                for flw, chunks_orig in chunks_orig_all.items():
                    chunks_selected_data[line][flw] = chunks_orig[chunk_mode]
            # Minimize writing to the database by updating "data" and the
            # database separately.
            data[chunks_selected_key] = chunks_selected_data
            rdb[key][chunks_selected_key] = chunks_selected_data

    rdb.close()
    return data


def plot_seq(data, fln, odr=path.join(PROGDIR, "..", "graphs"),
             ins=None, flt=lambda idx, label: True, order=None, xlm=None,
             ylm=None, chunk_mode=None):
    voq_lens_all = None
    if chunk_mode is None or chunk_mode == "agg":
        # Plot aggregate metrics for all chunks from all flows in each
        # experiment.
        ys = data["seqs"]
        xs = [xrange(len(ys[i])) for i in xrange(len(ys))]
        keys = data["keys"]
    else:
        # Include the "optimal" and "packet only" lines.
        lines = [
            ([x for x in xrange(len(ys))], ys) for ys in data["seqs"][0:2]]
        if chunk_mode == "best":
            # Plot the best chunk from any flow in each experiment.
            lines.extend(data["chunks_best"].values())
            keys = data["keys"]
        else:
            # Plot a specific chunk from all flows in a single experiment. Do
            # not use a legend because each line will correspond to a separate
            # flow instead of a separate experiment.
            exps_data = data["chunks_selected_chunk{}".format(chunk_mode)]
            assert len(exps_data) == 1, \
                ("There should be exactly one experiment when running "
                 "chunk_mode={}").format(chunk_mode)
            # Extracts a flow's src ID.
            get_src_id = lambda f: int(f.split(".")[-1])
            # Sort the results by the flow src ID, then split the flows and
            # their results. Do ".values()[0]" because we assume that, if we are
            # here, then there is only a single experiment (see above).
            flws, flw_lines = zip(*sorted(
                exps_data.values()[0].items(),
                key=lambda p: get_src_id(p[0][0])))

            # Combine the separate VOQ length results. This assumes that all of
            # the flows were sent from the same rack to the same rack. Start by
            # extracting the xs and VOQ lengths for each flow and turning them
            # into single-point pairs.
            voq_lens_all = [zip(xs, voq_lens) for xs, _, voq_lens in flw_lines]
            # Flatten the per-flow VOQ length results into one master list.
            voq_lens_all = [pair for voq_lens_flw in voq_lens_all for pair in voq_lens_flw]
            # Sort the VOQ length results by their x-values.
            voq_lens_all = sorted(voq_lens_all, key=lambda val: val[0])
            # Split the xs and ys into separate lists.
            voq_lens_all = zip(*voq_lens_all)

            # Remove the VOQ lengths from the results.
            flw_lines = [(xs, ys) for xs, ys, _ in flw_lines]
            lines.extend(flw_lines)

            # Convert each of the flows from a src flow ID.
            keys = (data["keys"][0:2] +
                    ["flow {}".format(get_src_id(flw[0])) for flw in flws])
        xs, ys = zip(*lines)

    # Format the legend labels.
    lls = []
    for k in keys:
        try:
            k_int = int(k)
            if "static" in fln:
                lls += ["%s packets" % k_int]
            else:
                lls += ["%s $\mu$s" % k_int]
        except ValueError:
            lls += [k]

    options = dotmap.DotMap()
    options.plot_type = "LINE" if chunk_mode is None else "SCATTER"
    options.legend.options.loc = "center right"
    options.legend.options.labels = lls
    options.legend.options.fontsize = 18
    options.output_fn = path.join(odr, "{}.pdf".format(fln))
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
    circuit_lines = data["circuit_lines"]
    options.vertical_lines.lines = circuit_lines
    shaded = []
    for i in xrange(0, len(circuit_lines), 2):
        shaded.append((circuit_lines[i], circuit_lines[i + 1]))
    options.vertical_shaded.limits = shaded
    options.vertical_shaded.options.alpha = 0.1
    options.vertical_shaded.options.color = "blue"
    if ins is not None:
        # Enable an inset.
        options.inset.show = True
        options.inset.options.zoom_level = 4
        options.inset.options.corners = (2, 3)
        options.inset.options.location = "center right"
        options.inset.options.marker.options.color = "black"
        xlm_ins, ylm_ins = ins
        options.inset.options.x.limits = xlm_ins
        options.inset.options.y.limits = ylm_ins

    if flt is not None:
        # Pick only the lines that we want.
        xs, ys, options.legend.options.labels = zip(
            *[(x, y, l) for (i, (x, y, l)) in enumerate(
                zip(xs, ys, options.legend.options.labels))
              if flt(i, l)])
    if order is not None:
        # Reorder the lines.
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

    # Set series options. Do this after filtering so that we have an accurate
    # count of the number of series.
    if chunk_mode is None:
        options.series_options = [
            dotmap.DotMap(linewidth=2) for _ in xrange(len(xs))]
    else:
        options.series_options = [
            dotmap.DotMap(s=6, edgecolors="none") for _ in xrange(len(xs))]
    # Set legend options. Do this after filtering so that we have an accurate
    # count of the number of series. Use 1 column if there are 10 or fewer
    # lines, otherwise use 2 columns.
    options.legend.options.ncol, options.legend.options.bbox_to_anchor = \
        (1, (1.4, 0.5)) if len(xs) <= 10 else (2, (1.65, 0.5))
    if voq_lens_all is not None:
        # If we are going to plot a second y-axis, then shift the legend to the
        # right.
        offset_x, offset_y = options.legend.options.bbox_to_anchor
        options.legend.options.bbox_to_anchor = (offset_x + 0.1, offset_y)

    simpleplotlib.plot(xs, ys, options)

    if voq_lens_all is not None:
        # Plot the VOQ length on a second y-axis. Modify the active figure
        # instance to include a totally separate line on a second y-axis. We
        # cannot use simpleplotlib's built-in y2 functionality because we are
        # not plotting a y2 line for each y line...we want only one y2 line.
        options2 = simpleplotlib.default_options.copy()
        options2.output_fn = options.output_fn
        options2.plot_type = "LINE"
        options2.series2_options = [dotmap.DotMap(
            linewidth=1, color="black", alpha=0.5, marker="o",
            markeredgecolor="none", markersize=3)]
        options2.x.limits = options.x.limits
        options2.x.margin = options2.y2.margin = \
            simpleplotlib.default_options.x.margin
        options2.y2.axis.color = options.y.axis.color
        options2.y2.label.fontsize = \
            options.y.label.fontsize
        options2.y2.label.ylabel = "VOQ length (packets)"
        options2.y2.ticks.major.options.labelsize = \
                options.y.ticks.major.options.labelsize

        ax2 = pyplot.gca().twinx()
        voq_xs, voq_ys = voq_lens_all
        simpleplotlib.plot_data(
            ax2, [voq_xs], [voq_ys], options2, options2.series2_options)
        simpleplotlib.apply_options_to_axis(ax2.xaxis, voq_xs, options2.x)
        simpleplotlib.apply_options_to_axis(ax2.yaxis, voq_ys, options2.y2)
        # Overwrite the original graph.
        pyplot.savefig(options2["output_fn"], bbox_inches="tight", pad_inches=0)


def seq(name, edr, odr, ptn, key_fnc, dur, ins=None, flt=None, order=None,
        xlm=None, ylm=None, chunk_mode=None, log_pos="after", msg_len=112,
        sync=False):
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
    chunk_mode: None, "best", or an integer
    log_pos: "before" or "after" the hybrid switch
    msg_len: The length of each HSLog message
    sync: True and False mean that the data parsing will be executed using a
          single thread and multiple threads, respectively.
    """
    print("Plotting: {}".format(name))
    parse_logs.DURATION = dur
    # Names are of the form "<number>_<details>_<specific options>". Experiments
    # where <details> are the same should be based on the same data. Therefore,
    # use <details> as the database key.
    if "_" in name:
        basename = name.split("_")[1]
    else:
        basename = name

    # If we are in chunk mode, then the parsing is slightly different and we use
    # a separate results database file.
    rdb_fln_ptn = "{}"
    if chunk_mode is not None:
        rdb_fln_ptn += "_chunk"
    rdb_fln_ptn += ".db"

    data = get_data(path.join(edr, rdb_fln_ptn.format(basename)), basename,
                    [ptn], dur, key_fnc, chunk_mode, log_pos, msg_len, sync)
    add_optimal(data)
    plot_seq(data, name, odr, ins, flt, order, xlm, ylm, chunk_mode)
    pyplot.close()
