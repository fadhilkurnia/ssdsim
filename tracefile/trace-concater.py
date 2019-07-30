#!/usr/bin/env python3
# script to make tracefile longer

import argparse,sys
from shutil import copyfile
import numpy as np

def main(argv):
    if len(argv) != 3:
        print('   Wrong usage!')
        print('   Usage: trace-concater <tracefile> <multiplier-num> <output-trace-filename>')
        print('   Ex: trace-concater tpcc.trace 3 tpcc_long.trace')
        return

    srcfile = argv[0].strip()
    multnum = int(argv[1])
    outfile = argv[2].strip()

    # get latest timestamp
    src = open(srcfile)
    lines = src.readlines()
    lasttimestamp = int(lines[-1].strip().split()[0])
    nrequest = len(lines)
    print(lasttimestamp)

    # concat
    otf = open(outfile, 'w')
    
    prev_time = 0
    int_arv_time = 0
    for i in range(multnum):
        offset_time = 0
        if i > 0:
            offset_time = lasttimestamp + int_arv_time
            offset_time = offset_time - int(lines[0].strip().split()[0])

        for j, line in enumerate(lines):
            linetoken = line.strip().split()
            timestamp = int(linetoken[0]) + offset_time
            diskid = linetoken[1]
            lsn = int(linetoken[2])
            size = linetoken[3]
            ope = linetoken[4]

            
            lsn = lsn + (i*16) # make lsn litle bit random

            otf.write(str(timestamp) + ' ' + diskid + ' ' + str(lsn) + ' ' + size + ' ' + ope + '\n')
            
            if j == nrequest-1:
                lasttimestamp = timestamp

            if i == 0 and j > 0:
                int_arv_time = int_arv_time + (timestamp-prev_time)
            if i == 0 and j == nrequest-1:
                print(int_arv_time)
                int_arv_time = int(int_arv_time/(nrequest-1))
                print(int_arv_time)
            if i == 0:
                prev_time = timestamp
   
    src.close()
    otf.close()

if __name__ == "__main__":
    main(sys.argv[1:])