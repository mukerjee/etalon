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
UNITS = 1000.


class FileReaderArgs(object):
    def __init__(self, dur, key, fln, time_offset_s, msg_len=112):
        self.dur = dur
        self.key = key
        self.fln = fln
        self.time_offset_s = time_offset_s
        self.msg_len = msg_len


class FileReader(object):
    def __call__(self, args):
        """
        Parses a single results file. "args" is a FileReaderArgs object. Returns
        tuples of the form (key, results).
        """
        print("Parsing: {}".format(args.fln))

        # Results with flow cleaning.
        results, bounds, _ = parse_logs.get_seq_data(
            args.fln, args.dur, args.time_offset_s, args.msg_len, clean=True)
        # Results without flow cleaning.
        _, _, chunks = parse_logs.get_seq_data(
            args.fln, args.dur, args.time_offset_s, args.msg_len, clean=False)
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
    bounds = [int(round(q)) for q in data["circuit_bounds"]]
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


def get_data(rdb_filepath, key, ptns, dur, key_fnc, time_offset_s,
             chunk_mode=None, msg_len=112, sync=False):
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
                dur, key_fnc(path.basename(fln)), fln, time_offset_s, msg_len)
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
        # raw_data is a list of tuples of the form:
        #   (key, (results, bounds, chunks))
        # Each entry corresponds to one line/experiment. The sorting is
        # important so that, in the final graphs, the mapping between lines and
        # legend is correct.
        data["raw_data"] = sorted(raw_data)
        # The first element that results from unzipping the raw data is a list
        # of the first entries from each line, which is a list of the keys for
        # the lines.
        data["keys"] = list(zip(*data["raw_data"])[0])
        # Extract the bounds of the first line.
        data["circuit_bounds"] = data["raw_data"][0][1][1]
        # First, extract all of the second elements (i.e., drop the keys). Then,
        # extract the results. Finally, extract the seqs and voqs results
        # themselves. The final result is a list of lists of results, where each
        # sublist corresponds to one line.
        seqs, voqs = zip(*zip(*zip(*data["raw_data"])[1])[0])
        # Convert the seqs to the correct units.
        data["seqs"] = [[sy / UNITS for sy in seq_ys]
                        for seq_ys in seqs]
        data["voqs"] = voqs

        # Convert the results for each set of original chunk data. Look through
        # each line.
        for line, (_, _, chunks_origs) in data["raw_data"]:
            # Check whether all flows have the same number of chunks. Extract
            # the number of chunks per flow.
            num_chunks = {flw: len(chunks_orig)
                          for flw, chunks_orig in chunks_origs.items()}
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
            for flw, chunks_orig in chunks_origs.items():
                data["chunks_orig"][line][flw] = []
                # Look through each chunk in this flow.
                for chunk_orig in chunks_orig:
                    seq_xs, seq_ys, voq_xs, voq_ys, chunk_idx = chunk_orig
                    data["chunks_orig"][line][flw].append(
                        (seq_xs, [sy / UNITS for sy in seq_ys], voq_xs, voq_ys,
                         chunk_idx))

        # Select the best chunk for each line (i.e., the chunk with the most
        # datapoints). Look through each line.
        for line, chunks_origs in data["chunks_orig"].items():
            data["chunks_best"][line] = ([], [], [])
            # Look through flow in this line.
            for chunks_orig in chunks_origs.values():
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
            for line, chunks_origs in data["chunks_orig"].items():
                chunks_selected_data[line] = {}
                for flw, chunks_orig in chunks_origs.items():
                    # Note that if the flows have different numbers of chunks,
                    # then the nth chunk for one flow may not correspond to the
                    # nth chunk from another flow. I.e., the chunks that we
                    # extract here may not be the same across flows. This is one
                    # of the reasons that if we are looking that the raw chunk
                    # analysis, then we should run the experiments with only a
                    # single flow.
                    chunks_selected_data[line][flw] = chunks_orig[chunk_mode]
            # Minimize writing to the database by updating "data" and the
            # database separately.
            data[chunks_selected_key] = chunks_selected_data
            rdb[key][chunks_selected_key] = chunks_selected_data

    rdb.close()
    return data


def plot_seq(data, fln, odr=path.join(PROGDIR, "..", "graphs"),
             ins=None, flt=lambda idx, label: True, order=None, xlm=None,
             ylm=None, chunk_mode=None, voq_agg=False):
    assert not voq_agg or chunk_mode is None, \
        "voq_agg=True requires chunk_mode=None!"

    plot_voqs = False
    if chunk_mode is None:
        # Plot aggregate metrics for all chunks from all flows in each
        # experiment.
        seq_ys = data["seqs"]
        seq_xs = [xrange(len(seq_ys[idx])) for idx in xrange(len(seq_ys))]
        keys = data["keys"]

        if voq_agg:
            # First, create a list of x values for each list of VOQ results.
            # Then, split apart the seq_xs and the seq_ys.
            voq_xs, voq_ys = zip(
                *[(xrange(len(voq_ys)), voq_ys) for voq_ys in data["voqs"]])
            plot_voqs = True
    else:
        # Include the "optimal" and "packet only" lines.
        lines = [([x for x in xrange(len(seq_ys))], seq_ys)
                 for seq_ys in data["seqs"][0:2]]
        if chunk_mode == "best":
            # Plot the best chunk from any flow in each experiment.
            lines.extend(data["chunks_best"].values())
            keys = data["keys"]
        else:
            # Plot a specific chunk from all flows in a single experiment. Do
            # not use a legend because each line will correspond to a separate
            # flow instead of a separate experiment.
            exps_data = data["chunks_selected_chunk{}".format(chunk_mode)]
            num_exps = len(exps_data)
            assert num_exps == 1, \
                ("When using chunk_mode={}, there should be exactly one "
                 "experiment, but there actually are: {}").format(
                     chunk_mode, num_exps)
            # Extracts a flow's src ID.
            get_src_id = lambda f: int(f.split(".")[-1])
            # Sort the results by the flow src ID, then split the flows and
            # their results. Do ".values()[0]" because we assume that, if we are
            # here, then there is only a single experiment (see above).
            flws, results_by_chunk = zip(*sorted(
                exps_data.values()[0].items(),
                key=lambda p: get_src_id(p[0][0])))

            # Combine the separate VOQ length results. This assumes that all of
            # the flows were sent from the same rack to the same rack. Start by
            # extracting the xs and VOQ lengths for each flow and turning them
            # into single-point pairs.
            voqs = [zip(voq_xs, voq_ys)
                    for _, _, voq_xs, voq_ys, _ in results_by_chunk]
            # Flatten the per-flow VOQ length results into one master list.
            voqs = [pair for voqs_flw in voqs for pair in voqs_flw]
            # Sort the VOQ length results by their x-values.
            voqs = sorted(voqs, key=lambda val: val[0])
            # Split the voq_xs and voq_ys into separate lists.
            voq_xs, voq_ys = zip(*voqs)
            # Since there is only a single experiment (see above) yet the rest
            # of the code is generalized to multiple experiments, wrap the
            # single experiment results in a list.
            voq_xs = [voq_xs]
            voq_ys = [voq_ys]
            plot_voqs = True

            # Remove the VOQ lengths from the results, then add them to the list
            # of all lines (which currently includes the "optimal" and "packet
            # only" lines).
            lines.extend([(seq_xs, seq_ys)
                          for seq_xs, seq_ys, _, _, _ in results_by_chunk])
            # Convert each of the flows to a src ID.
            keys = (data["keys"][0:2] +
                    ["flow {}".format(get_src_id(flw[0])) for flw in flws])
        seq_xs, seq_ys = zip(*lines)

    if plot_voqs:
        # Convert from tuples to lists to enable insertion.
        voq_xs = list(voq_xs)
        voq_ys = list(voq_ys)
        # Insert empty lists for the "optimal" and "packet only" lines. These
        # will be removed later.
        voq_xs.insert(0, None)
        voq_xs.insert(0, None)
        voq_ys.insert(0, None)
        voq_ys.insert(0, None)
    else:
        voq_xs = [None for _ in seq_xs]
        voq_ys = [None for _ in seq_ys]

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
    circuit_bounds = data["circuit_bounds"]
    options.vertical_lines.lines = circuit_bounds
    shaded = []
    for i in xrange(0, len(circuit_bounds), 2):
        shaded.append((circuit_bounds[i], circuit_bounds[i + 1]))
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
        seq_xs, seq_ys, voq_xs, voq_ys, options.legend.options.labels = zip(
            *[(sx, sy, vx, vy, l) for (idx, (sx, sy, vx, vy, l)) in enumerate(
                zip(seq_xs, seq_ys, voq_xs, voq_ys,
                    options.legend.options.labels))
              if flt(idx, l)])
    if order is not None:
        # Reorder the lines.
        real_seq_xs = []
        real_seq_ys = []
        real_voq_xs = []
        real_voq_ys = []
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
                real_seq_xs.append(seq_xs[idx])
                real_seq_ys.append(seq_ys[idx])
                real_voq_xs.append(voq_xs[idx])
                real_voq_ys.append(voq_ys[idx])
                real_ls.append(options.legend.options.labels[idx])
        seq_xs = real_seq_xs
        seq_ys = real_seq_ys
        voq_xs = real_voq_xs
        voq_ys = real_voq_ys
        options.legend.options.labels = real_ls

    # Set series options. Do this after filtering so that we have an accurate
    # count of the number of series.
    if chunk_mode is None:
        options.series_options = [
            dotmap.DotMap(color="C{}".format(idx), linewidth=2)
            for idx in xrange(len(seq_xs))]
    else:
        options.series_options = [
            dotmap.DotMap(s=6, edgecolors="none") for _ in xrange(len(seq_xs))]
    # Set legend options. Do this after filtering so that we have an accurate
    # count of the number of series. Use 1 column if there are 10 or fewer
    # lines, otherwise use 2 columns.
    options.legend.options.ncol, options.legend.options.bbox_to_anchor = \
        (1, (1.4, 0.5)) if len(seq_xs) <= 10 else (2, (1.65, 0.5))
    if plot_voqs:
        # If we are going to plot a second y-axis, then shift the legend to the
        # right.
        offset_x, offset_y = options.legend.options.bbox_to_anchor
        options.legend.options.bbox_to_anchor = (offset_x + 0.1, offset_y)

    simpleplotlib.plot(seq_xs, seq_ys, options)

    if plot_voqs:
        # Plot the VOQ length on a second y-axis. Modify the active figure
        # instance to include a totally separate line on a second y-axis. We
        # cannot use simpleplotlib's built-in y2 functionality because we are
        # not plotting a y2 line for each y line.
        options2 = simpleplotlib.default_options.copy()
        options2.output_fn = options.output_fn
        options2.plot_type = "LINE"
        options2.series2_options = [dotmap.DotMap() for _ in voq_xs]
        if voq_agg:
            # Configure the VOQ line to be dashed and the same color as the
            # corresponding sequence number lines.
            for idx in xrange(len(options2.series2_options)):
                options2.series2_options[idx].color = \
                    options.series_options[idx].color
                options2.series2_options[idx].linestyle = "dashed"
        else:
            # We are in chunk_mode==#, so there is only a single
            # experiment.
            options2.series2_options[-1].linewidth = 1
            options2.series2_options[-1].color = "black"
            options2.series2_options[-1].alpha = 0.5
            options2.series2_options[-1].marker = "o"
            options2.series2_options[-1].markeredgecolor = "none"
            options2.series2_options[-1].markersize = 3

        options2.x.limits = options.x.limits
        options2.x.margin = options2.y2.margin = \
            simpleplotlib.default_options.x.margin
        options2.y2.axis.color = options.y.axis.color
        options2.y2.label.fontsize = \
            options.y.label.fontsize
        options2.y2.label.ylabel = "VOQ length (packets)"
        options2.y2.ticks.major.options.labelsize = \
                options.y.ticks.major.options.labelsize

        # As a final step, remove the VOQ lines corresponding to "optimal" and
        # "packet only", which are None.
        voq_xs, voq_ys, options2.series2_options = zip(
            *[(vx, vy, o)
              for vx, vy, o in zip(voq_xs, voq_ys, options2.series2_options)
              if vx is not None])

        ax2 = pyplot.gca().twinx()
        simpleplotlib.plot_data(
            ax2, voq_xs, voq_ys, options2, options2.series2_options)
        simpleplotlib.apply_options_to_axis(ax2.xaxis, voq_xs, options2.x)
        simpleplotlib.apply_options_to_axis(ax2.yaxis, voq_ys, options2.y2)
        # Overwrite the original graph.
        pyplot.savefig(options2["output_fn"], bbox_inches="tight", pad_inches=0)


def seq(name, edr, odr, ptn, key_fnc, dur, cir_lat_s, ins=None, flt=None, order=None,
        xlm=None, ylm=None, chunk_mode=None, voq_agg=False, log_pos="after",
        msg_len=112, sync=False):
    """ Create a sequence graph.

    name: Name of this experiment, which become the output filename.
    edr: Experiment dir.
    odr: Output dir.
    ptn: Glob pattern for experiment files.
    key_fnc: Function that takes an experiment data filename returns a legend
             key.
    dur: The duration of the experiment, in milliseconds.
    cir_lat_s: The one-way latency of the circuit network, in microseconds.
                Used to offset the graphs to sequence numbers when packets
                arrived at the hybrid switch, even though the HSLog element is
                after the hybrid switch.
    ins: An inset specification.
    flt: Function that takes a legend index and label and returns a boolean
         indicating whether to include that line.
    order: List of the legend labels in their desired order.
    xlm: x-axis limits
    ylm: y-axis limits
    chunk_mode: None, "best", or an integer
    voq_agg: Whether to include aggregate VOQ length results on a second y-axis.
             True requires chunk_mode=None.
    log_pos: The location of the HSLog element: either "before" or "after" the
             hybrid switch.
    msg_len: The length of each HSLog message
    sync: True and False mean that the data parsing will be executed using a
          single thread and multiple threads, respectively.
    """
    print("Plotting: {}".format(name))
    parse_logs.DURATION = dur
    # Names are of the form "<number>_<details>_<specific options>". Experiments
    # where <details> are the same should be based on the same data. Therefore,
    # use <details> as the database key.
    basename = name.split("_")[1] if "_" in name else name
    data = get_data(
        rdb_filepath=path.join(edr, "{}.db".format(basename)),
        key=basename,
        ptns=[ptn],
        dur=dur,
        key_fnc=key_fnc,
        time_offset_s=cir_lat_s if log_pos == "after" else 0,
        chunk_mode=chunk_mode,
        msg_len=msg_len,
        sync=sync)
    add_optimal(data)
    plot_seq(data, name, odr, ins, flt, order, xlm, ylm, chunk_mode, voq_agg)
    pyplot.close()
