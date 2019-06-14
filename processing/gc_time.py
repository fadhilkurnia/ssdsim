#!/usr/bin/env python3
# script to plot number of GC over simulation time
# need generated statistic file from RAID's simulation

import sys, argparse
import matplotlib.pyplot as plt
import numpy as np

def cdf(x, plot=True, *args, **kwargs):
    x, y = sorted(x), np.arange(len(x)) / len(x)
    return plt.plot(x, y, *args, **kwargs) if plot else (x, y)

def raid_main(argv):
    raid_log = argv[0]
    numgc = 0
    
    # calculating the number of disk from RAID's log
    ndisk = 0
    for _ in open(raid_log):
        ndisk = ndisk + 1

    fig = plt.figure()
    graph = fig.add_axes((0.14, 0.21, 0.8, 0.7))

    # iterate through all the statistic file
    for diskid, ssd_gc_log in enumerate(open(raid_log)):
        data_numgc = []
        crt_second = 0
        ssd_numgc = 0

        for line in open(ssd_gc_log.strip() + 'gc.dat'):
            xtime = float(line.split()[6])/1000000000.0
            ssd_numgc = ssd_numgc + 1

            # handle first data
            if len(data_numgc) == 0:
                for _ in range(0, int(xtime)):
                    data_numgc.append(0)
                data_numgc.append(1)
                crt_second = int(xtime)
                continue

            # accumulate data if still in the same second
            if int(xtime) == crt_second:
                data_numgc[crt_second] = data_numgc[crt_second] + 1
                continue

            # jump to the new second
            if int(xtime) > crt_second:
                for _ in range(crt_second+1, int(xtime)):
                    data_numgc.append(0)
                data_numgc.append(1)
                crt_second = int(xtime)

        numgc = numgc + ssd_numgc
        print("#GC disk" + str(diskid) + ": " + str(ssd_numgc))

        # plot graph for this disk
        graph.plot(data_numgc, label='gc at disk'+str(diskid))

    graph.grid()
    graph.legend()

    print('Total number of GC: ' + str(numgc))

    # drawing the graph
    graph.set_title("Number of GC over time on RAID5-TPCC's workload\n(gc_threshold=0.10 aged_ratio=0.89 intensified by 10)")
    graph.set_ylabel("#GC")
    graph.set_xlabel("time (s)")

    # drawing cdf graph
    # plt.title("CDF of #GC/s on RAID5 TPCC")
    # plt.ylabel("CDF")
    # plt.xlabel("#GC")
    # plt.hist(data_numgc, len(data_numgc)-1, density=True, histtype='step', cumulative=True, label='Reversed emp.')
    # plt.margins(0)

    # fig.text(.5, .05, "Graph 1. The Cumulative Distributed Function (CDF) of #GC/s\nfrom vanilla TPCC simulation ", ha='center')
    fig.text(.5, .05, "Graph 1. Number of GC/s over time in RAID5 TPCC simulation", ha='center')
    plt.show()

    return

def cdf_gcgraph(logfile, israid):

    return

def raid_gcgraph(raidlogfilename):
    raid_log = raidlogfilename
    numgc = 0
    
    # calculating the number of disk from RAID's log
    ndisk = 0
    for _ in open(raid_log):
        ndisk = ndisk + 1

    fig = plt.figure()
    graph = fig.add_axes((0.14, 0.21, 0.8, 0.7))

    # iterate through all the statistic file
    for diskid, ssd_gc_log in enumerate(open(raid_log)):
        data_numgc = []
        crt_second = 0
        ssd_numgc = 0

        for line in open(ssd_gc_log.strip() + 'gc.dat'):
            xtime = float(line.split()[6])/1000000000.0
            ssd_numgc = ssd_numgc + 1

            # handle first data
            if len(data_numgc) == 0:
                for _ in range(0, int(xtime)):
                    data_numgc.append(0)
                data_numgc.append(1)
                crt_second = int(xtime)
                continue

            # accumulate data if still in the same second
            if int(xtime) == crt_second:
                data_numgc[crt_second] = data_numgc[crt_second] + 1
                continue

            # jump to the new second
            if int(xtime) > crt_second:
                for _ in range(crt_second+1, int(xtime)):
                    data_numgc.append(0)
                data_numgc.append(1)
                crt_second = int(xtime)

        numgc = numgc + ssd_numgc
        print("#GC disk" + str(diskid) + ": " + str(ssd_numgc))

        # plot graph for this disk
        graph.plot(data_numgc, label='gc at disk'+str(diskid))

    graph.grid()
    graph.legend()

    print('Total number of GC: ' + str(numgc))

    # drawing the graph
    graph.set_title("Number of GC over time on RAID5-TPCC's workload\n(gc_threshold=0.10 aged_ratio=0.89 intensified by 10)")
    graph.set_ylabel("#GC")
    graph.set_xlabel("time (s)")

    fig.text(.5, .05, "Graph 1. Number of GC/s over time in RAID5 TPCC simulation", ha='center')
    plt.show()
    return

def single_gcgraph(gclogfilename):
    fig = plt.figure()
    graph = fig.add_axes((0.14, 0.21, 0.8, 0.7))

    data_numgc = []
    crt_second = 0
    ssd_numgc = 0

    for line in open(gclogfilename):
        xtime = float(line.split()[6])/1000000000.0
        ssd_numgc = ssd_numgc + 1

        # handle first data
        if len(data_numgc) == 0:
            for _ in range(0, int(xtime)):
                data_numgc.append(0)
            data_numgc.append(1)
            crt_second = int(xtime)
            continue

        # accumulate data if still in the same second
        if int(xtime) == crt_second:
            data_numgc[crt_second] = data_numgc[crt_second] + 1
            continue

        # jump to the new second
        if int(xtime) > crt_second:
            for _ in range(crt_second+1, int(xtime)):
                data_numgc.append(0)
            data_numgc.append(1)
            crt_second = int(xtime)
        
    print("Total #GC: " + str(ssd_numgc))
    graph.plot(data_numgc, label='#GC/s')

    # drawing the graph
    graph.grid()
    graph.legend()
    graph.set_title("Number of GC over time on TPCC's workload\n(gc_threshold=0.10 less intensified by 10)")
    graph.set_ylabel("#GC")
    graph.set_xlabel("time (s)")
    fig.text(.5, .05, "Graph 2. Number of GC/s over time from a single SSD simulation", ha='center')
    plt.show()

    return

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Create #GC/s graph from generated log')
    parser.add_argument('logid', type=str, help='For single disk simulation, it is the timestamp when the simulation is started. Please end it with "/"!. Raid simulation log is started with "raid_"')
    parser.add_argument('-r','--raid', action='store_true', help='Set true for #GC/s graph from raid simulation')
    parser.add_argument('-c','--cdf', action='store_true', help='Set true for make CDF of #GC/s graph')
    args = parser.parse_args()

    if args.cdf:
        cdf_gcgraph(args.logid, args.raid)
    elif args.raid:
        raid_gcgraph(args.logid)
    else:
        single_gcgraph(args.logid + "gc.dat")


