#!/usr/bin/env python3
# script to make tracefile longer

import argparse,sys
from shutil import copyfile

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
    print(lasttimestamp)

    # concat
    otf = open(outfile, 'w')
    
    for i in range(multnum):
        for line in lines:
            linetoken = line.strip().split()
            timestamp = int(linetoken[0]) + (i * lasttimestamp)
            diskid = linetoken[1]
            lsn = linetoken[2]
            size = linetoken[3]
            ope = linetoken[4]

            otf.write(str(timestamp) + ' ' + diskid + ' ' + lsn + ' ' + size + ' ' + ope + '\n')
   
    src.close()
    otf.close()

if __name__ == "__main__":
    main(sys.argv[1:])