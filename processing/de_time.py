#!/usr/bin/env python3
import matplotlib.pyplot as plt
import numpy as np
import sys

# Direct erase on RAID SSD GC, input: raid's simulation log

def main(argv):
    raid_log = argv[0]

    ndisk = 0
    for ssd_gc_log in open(raid_log):
        ndisk = ndisk + 1

    # prepare graph
    fig = plt.figure()
    graph = fig.add_axes((0.14, 0.21, 0.8, 0.7))

    for diskid, ssd_gc_log in enumerate(open(raid_log)):
        data_de = []
        prev_de = 0
        crt_second = 0

        for line in open(ssd_gc_log.strip() + 'gc.dat'):
            xtime = float(line.split()[10])/1000000000.0
            
            cum_direct_erase = float(line.split()[16])
            crt_de = cum_direct_erase-prev_de
            prev_de = cum_direct_erase

            # handle first data
            if len(data_de) == 0:
                for _ in range(0, int(xtime)):
                    data_de.append(crt_de)
                data_de.append(crt_de)
                crt_second = int(xtime)
                continue

            # accumulate data
            if int(xtime) == crt_second:
                data_de[crt_second] = data_de[crt_second] + crt_de

            if int(xtime) > crt_second:
                for _ in range(crt_second+1, int(xtime)):
                    data_de.append(data_de[crt_second])
                crt_second = int(xtime)
                data_de.append(crt_de)
                continue

        # plot graph for diskid
        graph.plot(data_de, label='drct_erase disk'+str(diskid))

    graph.grid()
    graph.legend()

    # drawung the graph
    # plt.plot(data)
    graph.set_title("Number of block_direct_erase operation on RAID5 TPCC simulation")
    # graph.set_ylim(0,3)
    graph.set_ylabel("#bk_drct_erase")
    graph.set_xlabel("time (s)")
    fig.text(.5, .05, "Graph 3. Number of direct_erase operaton per second in RAID5 TPCC simulation", ha='center')
    plt.show()

    return

if __name__ == "__main__":
    main(sys.argv[1:])
