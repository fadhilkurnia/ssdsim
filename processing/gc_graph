#!/usr/bin/env python3
import sys, argparse, os, glob
import numpy as np
import matplotlib.pyplot as plt

# Hasilkan graf GC berdasarkan semua file yang ada dalam directory gc_graph_input
# fil input berformat .dat, dalam setiap file input merupakan log dari GC
# pada setiap channel atau setiap disk

INPUT_DIR = 'processing/gc_graph_input/'

def main(argv):
    if len(argv) > 0 and argv[0] == "-preprocess":
        if len(argv) < 3:
            raise ValueError("ERROR: Preprocess need log filename and output directory as the arguments")
        
        preprocess_logfile(argv[1], argv[2])
    else:
        generate_graph(argv)


# break a gc logfile into multiple logfiles, so a logfile only contain
# gc log for certain channel
def preprocess_logfile(infile, outdir):
    logfile = open(infile, 'r')
    
    # get input log short filename
    raw_infile = infile.split('/')[-1]
    raw_infile_token = raw_infile.split('.')
    raw_infile = ""
    for token in raw_infile_token:
        raw_infile += token
    
    output_files = {}
    for line in logfile:
        line_token = line.split()
        channel = line_token[0]
        if channel in output_files:
            output_files[channel].write(line)
        else:
            output_files[channel] = open("{0}{1}_{2}.dat".format(outdir, raw_infile, channel), 'w+')
            output_files[channel].write(line)
        
    for _, value in output_files.items():
        value.close()
    logfile.close()

# generate the graph from all logfiles in log_directory
def generate_graph(argv):
    # optional args: graph_title, start_time, end_time)

    parser = argparse.ArgumentParser()
    parser.add_argument('--title', type=str, help='Title of the graph')
    parser.add_argument('--min', type=int, help='Lower bound of time in ms that want to be shown in the graph')
    parser.add_argument('--max', type=int, help='Upper bound of time in ms that want to be shown in the graph')
    args = parser.parse_args()

    if args.title is None:
        args.title = "Record of GC Processes during SSD Simulation"

    # read all file in input directory
    log_files = glob.glob(INPUT_DIR + '*.dat')

    if len(log_files) == 0 :
        raise ValueError("ERROR: Whether input directory doesn't exist or empty!")

    # analyzing the GCs, find the min and the max time
    min_time = None
    max_time = 0
    for filename in log_files:
        data = np.loadtxt(filename)
        for gc in data:
            start_time = int(gc[6]/1000000)
            end_time = int(gc[7]/1000000)

            if min_time is None or min_time > start_time:
                min_time = start_time
            if max_time < end_time:
                max_time = end_time

    print("min time ", min_time, " ms")
    print("max time ", max_time, " ms")
    if args.min is None or args.min < min_time:
        args.min = min_time
    if args.max is None or args.max > max_time:
        args.max = max_time
    
    # preparing ytics label
    yticks_label = {}
    for filename in log_files:
        short_filename = filename.split('/')[-1]
        short_filename = short_filename[:-4] # remove .dat
        yticks_label[filename] = short_filename

    # creating the graph
    fig = plt.figure()
    plt.grid()
    plt.title(args.title)
    plt.xlabel("time (ms)")
    for filename, short_filename in yticks_label.items():
        _, _, _, _, _, _, s_gc, f_gc, _ = np.loadtxt(filename, unpack=True)
        s_gc = s_gc / 1000000
        f_gc = f_gc / 1000000
        yticks = [short_filename] * len(s_gc)
        plt.hlines(yticks, s_gc, f_gc, colors="red", lw=20)
    plt.xlim(args.min, args.max)

    # displaying graph
    print("\n")
    plt.show()

    print("Finish generating graph")

if __name__ == "__main__":
    main(sys.argv[1:])