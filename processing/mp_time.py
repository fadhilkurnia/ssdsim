#!/usr/bin/env python3
# script to plot number of moved page during GC over simulation time
# need generated statistic file from RAID's simulation

import sys, argparse
import matplotlib.pyplot as plt

def main(argv):
    raid_log = argv[0]
    
    # calculating the number of disk from RAID's log
    ndisk = 0
    for _ in open(raid_log):
        ndisk = ndisk + 1

    fig = plt.figure()
    graph = fig.add_axes((0.14, 0.21, 0.8, 0.7))

    # iterate through all the statistic file from each disk
    for diskid, ssd_gc_log in enumerate(open(raid_log)):
        data_mvdpage = []
        crt_second = 0
        num_gc = 0
        num_mvdpg = 0

        for line in open(ssd_gc_log.strip() + 'gc.dat'):
            xtime = float(line.split()[6])/1000000000.0
            mvdpage = int(line.split()[5])
            
            num_mvdpg = num_mvdpg + mvdpage
            num_gc = num_gc + 1

            # handle first data
            if len(data_mvdpage) == 0:
                for _ in range(0, int(xtime)):
                    data_mvdpage.append(0)
                data_mvdpage.append(mvdpage)
                crt_second = int(xtime)
                continue

            # accumulate data if still in the same second
            if int(xtime) == crt_second:
                data_mvdpage[crt_second] = data_mvdpage[crt_second] + mvdpage
                continue

            # jump to the new second
            if int(xtime) > crt_second:
                for _ in range(crt_second+1, int(xtime)):
                    data_mvdpage.append(0)
                data_mvdpage.append(mvdpage)
                crt_second = int(xtime)

        # plot graph for this disk
        graph.plot(data_mvdpage, label='mvd page at disk'+str(diskid))

        # print average moved-page per GC for this disk
        print("disk-" + str(diskid) + ": " + str(float(num_mvdpg)/float(num_gc)))

    graph.grid()
    graph.legend()

    # drawing the graph
    graph.set_title("Number of moved page on GC process over time from \nRAID5 TPCC normal simulation")
    graph.set_ylabel("# moved-page")
    graph.set_xlabel("time (s)")
    fig.text(.5, .05, "Graph 2. Number of moved-page/s over time in RAID5 vanilla TPCC simulation", ha='center')
    plt.show()

    return

def raid_mpgraph(raidlogfilename):
    raid_log = raidlogfilename
    
    # calculating the number of disk from RAID's log
    ndisk = 0
    for _ in open(raid_log):
        ndisk = ndisk + 1

    fig = plt.figure()
    graph = fig.add_axes((0.14, 0.21, 0.8, 0.7))

    # iterate through all the statistic file from each disk
    for diskid, ssd_gc_log in enumerate(open(raid_log)):
        data_mvdpage = []
        crt_second = 0
        num_gc = 0
        num_mvdpg = 0

        for line in open(ssd_gc_log.strip() + 'gc.dat'):
            xtime = float(line.split()[6])/1000000000.0
            mvdpage = int(line.split()[5])
            
            num_mvdpg = num_mvdpg + mvdpage
            num_gc = num_gc + 1

            # handle first data
            if len(data_mvdpage) == 0:
                for _ in range(0, int(xtime)):
                    data_mvdpage.append(0)
                data_mvdpage.append(mvdpage)
                crt_second = int(xtime)
                continue

            # accumulate data if still in the same second
            if int(xtime) == crt_second:
                data_mvdpage[crt_second] = data_mvdpage[crt_second] + mvdpage
                continue

            # jump to the new second
            if int(xtime) > crt_second:
                for _ in range(crt_second+1, int(xtime)):
                    data_mvdpage.append(0)
                data_mvdpage.append(mvdpage)
                crt_second = int(xtime)

        # plot graph for this disk
        graph.plot(data_mvdpage, label='mvd page at disk'+str(diskid))

        # print average moved-page per GC for this disk
        print("disk-" + str(diskid) + ": " + str(float(num_mvdpg)/float(num_gc)))

    graph.grid()
    graph.legend()

    # drawing the graph
    graph.set_title("Number of moved page on GC process over time from \nRAID5 TPCC normal simulation")
    graph.set_ylabel("# moved-page")
    graph.set_xlabel("time (s)")
    fig.text(.5, .05, "Graph 2. Number of moved-page/s over time in RAID5 vanilla TPCC simulation", ha='center')
    plt.show()
    return

def single_mpgraph(gclogfilename, offset):
    fig = plt.figure()
    graph = fig.add_axes((0.14, 0.21, 0.8, 0.7))

    data_mvdpage = []
    crt_second = 0
    num_mvdpg = 0
    num_gc = 0
    for line in open(gclogfilename):
        xtime = float(line.split()[6])/1000000000.0
        mvdpage = int(line.split()[5])
        
        if int(line.split()[6]) > offset:
            num_mvdpg = num_mvdpg + mvdpage
            num_gc = num_gc + 1

        # handle first data
        if len(data_mvdpage) == 0:
            for _ in range(0, int(xtime)):
                data_mvdpage.append(0)
            data_mvdpage.append(mvdpage)
            crt_second = int(xtime)
            continue

        # accumulate data if still in the same second
        if int(xtime) == crt_second:
            data_mvdpage[crt_second] = data_mvdpage[crt_second] + mvdpage
            continue

        # jump to the new second
        if int(xtime) > crt_second:
            for _ in range(crt_second+1, int(xtime)):
                data_mvdpage.append(0)
            data_mvdpage.append(mvdpage)
            crt_second = int(xtime)
    

    # print average moved-page per GC
    print("Avg. moved page per GC: " + str(float(num_mvdpg)/float(num_gc)))
    
    # drawing the graph
    graph.plot(data_mvdpage, label='#mvd-page/s')
    graph.grid()
    graph.legend()
    graph.set_title("Number of moved page on GC process over time from \nTPCC normal simulation")
    graph.set_ylabel("# moved-page")
    graph.set_xlabel("time (s)")
    fig.text(.5, .05, "Graph 3. Number of moved-page/s over time from a single SSD TPCC simulation", ha='center')
    plt.show()

    return

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description='Create #MP/s graph from generated log')
    parser.add_argument('logid', type=str, help='For single disk simulation, it is the timestamp when the simulation is started. Please end it with "/"!. Raid simulation log is started with "raid_"')
    parser.add_argument('-r','--raid', action='store_true', help='Set true for #GC/s graph from raid simulation')
    parser.add_argument('--offset', type=int, default=0, help='Starting time offset')
    args = parser.parse_args()

    if args.raid:
        raid_mpgraph(args.logid)
    else:
        single_mpgraph(args.logid + "gc.dat", args.offset)