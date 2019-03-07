#!/usr/bin/env python

import sys
from matplotlib import pyplot as plt

# need raid log filename
# tested with python 3
def main(argv):
    if len(argv) < 1:
        raise ValueError("Need raid log filename, it can be obtained after runing raid simulation with raid5 script")

    raidlog_filename = argv[0]
    raidlog = open(raidlog_filename, "r")
    
    iolog_filename = []
    iolog_file = []

    iodata = dict()

    for line in raidlog:
        line = line.rstrip()
        iolog_filename.append(line + "io_read.dat")
        iolog_file.append(open(line + "io_read.dat", "r"))

    ndisk = len(iolog_filename)
    io_collision = [0] * (ndisk + 1)

    # get collision data per disk
    for idx, iofile in enumerate(iolog_file):
        for line in iofile:
            io_token = line.split()
            io_start = int(io_token[0])
            io_address = int(io_token[1])
            io_size = int(io_token[2])
            io_ope = int(io_token[3])
            io_gc_flag = int(io_token[7])
            io_gc_remaining_time = int(io_token[8])

            io_key = str(io_start) + "_" + str(io_ope)
            
            if io_key not in iodata:
                iodata[io_key] = [0] * ndisk
            
            if io_gc_flag == 1:
                iodata[io_key][idx] = iodata[io_key][idx] + 1

    # get collision data per request
    iodata_collision = dict()
    for req_id in iodata:
        n_collision = 0
        for i in range(ndisk):
            if iodata[req_id][i] != 0:
                n_collision = n_collision + 1
        iodata_collision[req_id] = n_collision
    
    # Plot the graph
    nrequests = len(iodata_collision)
    for req in iodata_collision:
        io_collision[iodata_collision[req]] = io_collision[iodata_collision[req]] + 1

    xlabel = []
    for x in range(len(io_collision)):
        xlabel.append(str(x) + 'GC')

    plt.bar(xlabel, io_collision)
    plt.suptitle("Number of GC Collision Per Read IO")
    plt.title("RAID5 SSD with %d disks" % ndisk)
    plt.yscale('log')
    plt.grid(True)
    plt.ylabel("# GC (logaritmic scale)")
    plt.xlabel("# GC Collision")

    print("\n STATISTIC:")
    print("total request: " + str(nrequests))
    for i, v in enumerate(io_collision):

        percentage = float(v)/float(nrequests)*float(100)
        print("%s:(%.3f%%) \t %d" % (xlabel[i], percentage, v))

        ypos = v
        if ypos < 25:
            ypos = 25
        
        bar_label = "%d (%.1f%%)" % (v, percentage)
        plt.text(i, ypos, bar_label, color='black', ha='center', fontsize=10)

    plt.show()

    return

if __name__ == "__main__":
    main(sys.argv[1:])