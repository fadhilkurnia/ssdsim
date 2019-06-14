#!/usr/bin/env python3
import sys, argparse, random
import matplotlib.pyplot as plt
import numpy as np

# A script to make IO latency CDF graph for a single SSD

parser = argparse.ArgumentParser(description='Make CDF of IO latency from a single SSD\'s simulation log')
parser.add_argument('logid', type=str, help='Directory of the simulation log')
parser.add_argument('--ymin', type=float, default=0.0, help='Minimum value for y axis (0.0-1.0), default=0.0')
parser.add_argument('--offset', type=int, default=0, help='Starting time offset')
args = parser.parse_args()

iolog = open(args.logid + '/io_read.dat', 'r')
data = []
for line in iolog:
    time = int(line.split()[0])
    if time < args.offset:
        continue
    
    latency = float(line.split()[6])/1000000 # convert ns to ms
    data.append(latency)

# prepare the data
data.sort()
num_bins = len(data)
counts, bin_edges = np.histogram (data, bins=num_bins)
cdf = np.cumsum (counts)

# print some data
print("- avg.latency: " + str(sum(data)/len(data)) + ' ms')
print("- max.latency: " + str(data[len(data)-1]) + ' ms')
print("- min.latency: " + str(data[0]) + ' ms')

# draw the graph
fig = plt.figure()
graph = fig.add_axes((0.14, 0.21, 0.8, 0.7))
graph.plot(bin_edges[1:], cdf/cdf[-1])
graph.grid()
graph.set_title("SSD I/O read latency")
graph.set_ylabel("CDF")
graph.set_xlabel("I/O latency (ms)")
graph.set_ylim(args.ymin, 1.0)
graph.set_xlim(0.0, data[len(data)-1])
fig.text(.5, .05, "Graph 1. Read I/O latency from a single SSD simulation", ha='center')
plt.show()