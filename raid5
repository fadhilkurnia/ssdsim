#!/usr/bin/env python3

# automatically break a tracefile and simulate raid
# input: tracefile
# - break that tracefile into multiple tracefiles, each for one ssd.
# - save the generated tracefile in a folder inside tracefile directory 
# - run the raid simulation using all the generated tracefiles
# - after finish, generate a single log indicating where is the statistics for every ssd in the raid simulation

import sys, math, os, datetime, argparse

IO_READ = 1
IO_WRITE = 0
OUT_TRACEFILE_DIR = 'tracefile/'
OUT_RAID_LOG_DIR = 'raw/'
RAID_SEGMENT_SIZE = 40960           # in byte
SSD_BLOCK_SIZE = 2048               # in byte
AVG_PARITY_CALCULATION_TIME = 1000  # in ns

# python raid <num_disk> <tracefile>
def main(argv):

    parser = argparse.ArgumentParser(description='SSDSim RAID5 args.')
    parser.add_argument('ndisk', type=int)
    parser.add_argument('tracefilename', type=str)
    parser.add_argument('--gcsync', action='store_true', default=False, help='Flag for activating GCSync mode')
    parser.add_argument('--gcsync_time_window', type=int, default=500000000, help='GCSync time window in ns, default = 500ms')
    args = parser.parse_args()

    ndisk = args.ndisk
    tracefilename = args.tracefilename

    raid_simulation_timestamp = getCurrentTimestamp()

    print("preparing tracefiles")
    raid_tracefiles_location = breakTracefile(raid_simulation_timestamp, tracefilename, ndisk, RAID_SEGMENT_SIZE)

    print("runing simulation")
    runRAID(raid_simulation_timestamp, raid_tracefiles_location, is_gcsync=args.gcsync, gcsync_time_window=args.gcsync_time_window)

    return

def breakTracefile(simulation_timestamp, infile, ndisk, segment_size):
    firstline = [True] * ndisk
    outtraces = []
    outtracesname = []

    # doing some preparation
    timestamp_dir = simulation_timestamp + "/"
    if not os.path.exists(OUT_TRACEFILE_DIR+timestamp_dir):
        os.makedirs(OUT_TRACEFILE_DIR+timestamp_dir)
    
    for i in range (0,ndisk):
        outfilename =  OUT_TRACEFILE_DIR + timestamp_dir + infile.split("/")[-1] + "-raid5-" + str(i) + ".trace"
        outtracesname.append(outfilename)
        outtraces.append(open(outfilename, "w"))

    blk_size = SSD_BLOCK_SIZE                       # or sector size in hdd (in bytes)
    blk_per_segment = segment_size / blk_size
    blk_per_stripe = blk_per_segment * (ndisk - 1)  # --- modf_1 --- ndisk -> (ndisk-1) (Sep.17 17:19)

    # ======== Begin Some Helper Functions ====================================
    def writeTrace(id, trace):
        if firstline[id] is False:
            outtraces[id].write("\n")
        else:
            firstline[id] = False
        outtraces[id].write(trace)

    def isParitySegment(stripe_id, segment_id):
        return stripe_id%ndisk == segment_id

    def forwardSegment(crnt_stripe, crnt_segment):
        crnt_segment = crnt_segment + 1
        if crnt_segment == ndisk:
            crnt_segment = 0
            crnt_stripe = crnt_stripe + 1
        return crnt_stripe, crnt_segment

    def forwardNonParitySegment(crnt_stripe, crnt_segment):
        crnt_stripe, crnt_segment = forwardSegment(crnt_stripe, crnt_segment)
        while isParitySegment(crnt_stripe, crnt_segment):
            crnt_stripe, crnt_segment = forwardSegment(crnt_stripe, crnt_segment)

    def traceReadRequest(initial_stripe_id, initial_segment_id, blk_offset, time, blkcount):
        crnt_stripe_id = initial_stripe_id
        crnt_segment_id = initial_segment_id
        while blkcount > 0:
            processed = 0
            if blk_offset + blkcount > blk_per_segment:
                processed = blk_per_segment - blk_offset
                blkcount = blkcount - processed
            else:
                processed = blkcount

            writeTrace(crnt_segment_id, "%ld %d %ld %d %d" % (time, 0, blk_offset, processed, IO_READ))

            # move to next segment
            blk_offset = 0
            crnt_stripe_id, crnt_segment_id = forwardNonParitySegment(crnt_stripe_id, crnt_segment_id)

        return

    # work still in progress
    def traceWriteRequest(initial_stripe_id, initial_segment_id, blk_offset, time, blkcount):
        processed_disk = [0] * ndisk
        crnt_stripe_id = initial_stripe_id
        crnt_segment_id = initial_segment_id
        return
    # ======== End Some Helper Functions ====================================

    intrace = open(infile, "r")
    for line in intrace:
        token = line.split()
        time = int(token[0])
        devno = int(token[1])
        blkno = int(token[2])
        blkcount = int(token[3])
        operation = int(token[4])

        # calculating new starting blkno
        target_stripe_id = int(blkno / (blk_per_segment * (ndisk-1)))
        blk_stripe_offset = (blkno + (blk_per_segment*target_stripe_id)) % blk_per_stripe
        parity_disk_id = target_stripe_id % ndisk
        target_disk_id = int(blk_stripe_offset / blk_per_segment)
        if parity_disk_id <= target_disk_id: # right shift 1 disk to jump over parity disk
            target_disk_id = target_disk_id + 1
        new_blkno = (target_stripe_id*blk_per_segment) + (blk_stripe_offset%blk_per_segment)

        # iterate blkcount
        current_disk_id = target_disk_id
        current_stripe_id = target_stripe_id
        current_blkno = new_blkno
        current_blkcount = blkcount
        next_segment_blk = (current_stripe_id+1)*blk_per_segment

        max_blkcount_segment = 0
        min_blkno_segment = current_blkno

        while blkcount > 0:
            current_blkcount = blkcount # --- add_1 --- (Sep.18 14:24)

            if current_blkcount + current_blkno > next_segment_blk:
                current_blkcount = next_segment_blk-current_blkno

            if operation == IO_WRITE:
                writeTrace(current_disk_id, "%ld %d %ld %d %d" % (time, devno, current_blkno, current_blkcount, 1))
            writeTrace(current_disk_id, "%ld %d %ld %d %d" % (time, devno, current_blkno, current_blkcount, operation))

            max_blkcount_segment = 0 # --- add_3 --- (Sep.18 20:05)
            if max_blkcount_segment < current_blkcount: 
                max_blkcount_segment = current_blkcount

            blkcount = blkcount - current_blkcount

            # write parity for last stripe
            if operation == IO_WRITE and blkcount <= 0 :
                writeTrace(current_stripe_id%ndisk, "%ld %d %ld %d %d" % (time, devno, min_blkno_segment, max_blkcount_segment, IO_READ))
                writeTrace(current_stripe_id%ndisk, "%ld %d %ld %d %d" % (time, devno, min_blkno_segment, max_blkcount_segment, IO_WRITE))
                break

            # move to next segment and skip parity segment
            while True:

                current_disk_id = current_disk_id + 1 # move to next disk

                if current_disk_id == ndisk: # current disk is rightmost disk of current stripe, so move to next stripe

                    if operation == IO_WRITE: # write parity before change to next stripe (only for write operation)
                        writeTrace(current_stripe_id%ndisk, "%ld %d %ld %d %d" % (time, devno, min_blkno_segment, max_blkcount_segment, IO_READ))
                        writeTrace(current_stripe_id%ndisk, "%ld %d %ld %d %d" % (time, devno, min_blkno_segment, max_blkcount_segment, IO_WRITE))

                    current_disk_id = 0 # start over from disk_0
                    current_stripe_id = current_stripe_id + 1 # move to next stripe

                if current_disk_id != current_stripe_id%ndisk: # current disk is not parity disk
                    current_blkno = current_stripe_id * blk_per_segment # head of blkno in a segment
                    min_blkno_segment = current_blkno
                    max_blkcount_segment = blk_per_segment # --- add_2 --- (Sep.18 16:38)
                    break

            next_segment_blk = (current_stripe_id+1)*blk_per_segment

    return outtracesname

def runRAID(simulation_timestamp, tracefiles_loc, is_gcsync = False, gcsync_time_window = 500000000):
    raidlog = open("raw/raid_"+simulation_timestamp+".log", "w")
    num_disk = len(tracefiles_loc)

    for diskid, intracefile in enumerate(tracefiles_loc):
        timestamp = getCurrentTimestamp()
        command = "./ssd --timestamp "+timestamp+" "+intracefile
        if is_gcsync:
            command = "./ssd --gcsync --ndisk "+str(num_disk)+" --diskid  "+str(diskid)+" --gc_time_window "+str(gcsync_time_window)+" --timestamp "+timestamp+" "+intracefile
        print(command)

        os.system(command)
        raidlog.writelines(OUT_RAID_LOG_DIR + timestamp + "/" + "\n")

    return

def getCurrentTimestamp():
    crt_timestamp = datetime.datetime.now()
    return crt_timestamp.strftime("%Y%m%d_%H%M%S")

if __name__ == "__main__":
    main(sys.argv[1:])

