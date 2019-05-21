#!/usr/bin/env python3
import matplotlib.pyplot as plt
import numpy as np
import sys

# Free block for RAID SSD (fb_raid), input: raid's simulation log

def main(argv):
    raid_log = argv[0]

    ndisk = 0
    for ssd_gc_log in open(raid_log):
        ndisk = ndisk + 1

    # prepare graph
    # fig, axar = plt.subplots(nrows=int(ndisk/2), ncols=int(ndisk/2))
    # fig.suptitle('Free Page & Free Block on GCSync RAID5 (tw=100ms)')
    # fig.subplots_adjust(wspace=0.4, hspace=0.4)

    for diskid, ssd_gc_log in enumerate(open(raid_log)):
        data_block = []
        data_block_ne = []
        data_page = []
        data_page_ne = []
        crt_second = 0
        for line in open(ssd_gc_log.strip() + 'gc.dat'):
            xtime = float(line.split()[10])/1000000000.0
            free_blk_prct = float(line.split()[11])
            free_blk_ne_prct = float(line.split()[14])
            free_pg_prct = float(line.split()[12])
            free_pg_ne_prct = float(line.split()[13])

            # handle first data
            if len(data_block) == 0:
                for _ in range(0, int(xtime)):
                    data_block.append(free_blk_prct)
                    data_page.append(free_pg_prct)
                    data_page_ne.append(free_pg_ne_prct)
                    data_block_ne.append(free_blk_ne_prct)
                data_block.append(free_blk_prct)
                data_block_ne.append(free_blk_ne_prct)
                data_page.append(free_pg_prct)
                data_page_ne.append(free_pg_ne_prct)
                crt_second = int(xtime)
                continue

            # get the largest free block & free page percentage for every second
            if int(xtime) == crt_second and data_block[crt_second] < free_blk_prct:
                data_block[crt_second] = free_blk_prct
            if int(xtime) == crt_second and data_block_ne[crt_second] < free_blk_ne_prct:
                data_block_ne[crt_second] = free_blk_ne_prct
            if int(xtime) == crt_second and data_page[crt_second] < free_pg_prct:
                data_page[crt_second] = free_pg_prct
            if int(xtime) == crt_second and data_page_ne[crt_second] < free_pg_ne_prct:
                data_page_ne[crt_second] = free_pg_ne_prct

            if int(xtime) > crt_second:
                for _ in range(crt_second+1, int(xtime)):
                    data_block.append(data_block[crt_second])
                    data_block_ne.append(data_block_ne[crt_second])
                    data_page.append(data_page[crt_second])
                    data_page_ne.append(data_page_ne[crt_second])
                crt_second = int(xtime)
                data_block.append(free_blk_prct)
                data_block_ne.append(free_blk_ne_prct)
                data_page.append(free_pg_prct)
                data_page_ne.append(free_pg_ne_prct)
                continue

        # plot graph for diskid
        plt.plot(data_block, label='fr_block disk'+str(diskid))
        # plt.plot(data_block_ne, label='fr_block_ne disk'+str(diskid))
        plt.plot(data_page, label='fr_pg disk'+str(diskid))
        # plt.plot(data_page_ne, label='fr_pg_ne disk'+str(diskid))

    plt.grid()
    plt.legend()

    # drawung the graph
    # plt.plot(data)
    plt.title("Free Page & Free Block on GCSync RAID5 (tw=100ms)")
    plt.ylim(0,40)
    plt.ylabel("percentage (%)")
    plt.xlabel("time (s)")
    plt.show()

    return

if __name__ == "__main__":
    main(sys.argv[1:])
