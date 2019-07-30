#!/usr/bin/env python3
import sys, argparse, random

# A script to make warmup workload before the real workload. The result of this 
# script is a new tracefile, it contains concatenated warmup workload and anotther given workload

parser = argparse.ArgumentParser(description='Make warmuped workload from tracefile for SSDSim')
parser.add_argument('trace', type=str, help='File name of the tracefile')
parser.add_argument('outtrace', type=str, help='File name of the new generated tracefile')
parser.add_argument('--maxlsn', default=483183808, type=int, help='Maximum lsn for a single SSD, default=483183820')
parser.add_argument('--sector', default=512, type=int, help='A single sector size in bytes, default=512')
parser.add_argument('--bs', default=2048, type=int, help="Number of sector per SSD's block, default=2048")
parser.add_argument('--seed', default=99, type=int, help='Integer seed for randomizer, default=99')
args = parser.parse_args()

outtrace = open(args.outtrace, 'w')

# create full 128KB write workload throughout all the LBA
print("> preparing full 128KB write workload")
write_size_sector = int(128*1024/args.sector)
ia_time =  50000000 #50ms
lba = 0
time = 0
while lba < args.maxlsn:
    outtrace.write(str(time) + ' 0 ' + str(lba) + ' ' + str(write_size_sector) + ' 0\n')
    lba = lba + write_size_sector
    time = time + ia_time
print("> full 128KB write workload is prepared, 0-" + str(int(time/1000000000)) + " second")

# create random 4KB write on each block, 4KB is 8 sectors or 1 page
print("> preparing random 4KB write workload")
# 4KB = 8 sectors = 1 page
# def is_align_page(lsn):
#     return lsn%8==0
# time = time + 1000000000 # add 1 second
# ia_time =  100000000 # 100ms
# for x in range(1000):
#     lsn = random.randint(0, args.maxlsn+1)
#     while not is_align_page(lsn):
#         lsn = random.randint(0, args.maxlsn+1)
#     outtrace.write(str(time) + ' 0 ' + str(lba) + ' ' + str(write_size_sector) + ' 0\n')

# write_size_sector = int(4*1024/args.sector) # 8 sectors
# divider = args.bs/write_size_sector         # 2048/8 = 256 ws
# lsn_lbound = 0
# time = time + 1000000000                    # add 1 second
# ia_time =  100000000                        # 100ms
# while lsn_lbound + args.bs < args.maxlsn:
#     # for x in range(int(divider*0.25)): # try to make write request to 25% of the block
#     lba = int((random.randint(0,divider) * write_size_sector) + lsn_lbound)
#     time = time + ia_time
#     outtrace.write(str(time) + ' 0 ' + str(lba) + ' ' + str(write_size_sector) + ' 0\n')
#     lsn_lbound = lsn_lbound + args.bs
print("> random 4KB write workload is prepared, till " + str(int(time/1000000000)) + " second")

# print("> preparing 4KB read to first page in all block")
# lsn_lbound = 0
# ia_time =  10000000 #10ms
# while lsn_lbound + args.bs < args.maxlsn:
#     lba = lsn_lbound
#     time = time + ia_time
#     outtrace.write(str(time) + ' 0 ' + str(lba) + ' ' + str(write_size_sector) + ' 1\n')
#     lsn_lbound = lsn_lbound + args.bs
# print("> random 4KB read workload is prepared, till " + str(int(time)) + " ns")

# concat with given tracefile
# print("> concatenating the warmup workload with " + args.trace)
# trace = open(args.trace, 'r')
# init_time = time + 1000000000 # add 1 second
# for line in trace:
#     time = int(line.split()[0]) + init_time
#     lba = line.split()[2]
#     size = line.split()[3]
#     ope = line.split()[4]
#     outtrace.write(str(time) + ' 0 ' + str(lba) + ' ' + str(size) + ' ' + str(ope) + '\n')
# print("> finish concatenating, end timestamp is "+ str(int(time/1000000000)) + " second")
print("> the new workload (" + args.outtrace + ") is ready!")