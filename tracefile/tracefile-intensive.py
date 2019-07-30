#!/usr/bin/env python3
# script to make tracefile longer

import argparse,sys
from shutil import copyfile

def main(argv):
    if len(argv) != 3:
        print('   Wrong usage!')
        print('   Usage: trace-intensive <tracefile> <intensived-by> <output-trace-filename>')
        print('   Ex: trace-intensive tpcc.trace 4 tpcc_long.trace')
        return

    srcfile = argv[0].strip()
    intnum = int(argv[1])
    outfile = argv[2].strip()

    # get latest timestamp
    src = open(srcfile)
    lines = src.readlines()

    # concat
    otf = open(outfile, 'w')
    
    for line in lines:
        linetoken = line.strip().split()
        timestamp = int(int(linetoken[0]) / intnum)
        diskid = linetoken[1]
        lsn = linetoken[2]
        size = linetoken[3]
        ope = linetoken[4]

        otf.write(str(timestamp) + ' ' + diskid + ' ' + lsn + ' ' + size + ' ' + ope + '\n')
   
    src.close()
    otf.close()

if __name__ == "__main__":
    main(sys.argv[1:])