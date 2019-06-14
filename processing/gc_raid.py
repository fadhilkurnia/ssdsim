#!/usr/bin/env python3
import matplotlib.pyplot as plt
import numpy as np
import sys

def main(argv):
    # gather gc data from raid log
    raid_log = argv[0]
    
    # Create data
    N = 500
    x = np.array([])
    y = np.array([])
    area = np.pi*3
    ndisk = 0

    for diskid, ssd_log in enumerate(open(raid_log)):
        gc_log = ssd_log.strip() + "gc.dat"
        s_gc, f_gc = np.loadtxt(gc_log, unpack=True, usecols=(6,7))
        disk_x = (f_gc - s_gc / 2) / 1000000.0
        disk_y = [diskid] * len(disk_x)
        
        x = np.append(x, disk_x)
        y = np.append(y, disk_y)

        ndisk = ndisk + 1

    # Plot
    plt.grid()
    plt.scatter(x, y, s=area, c='b', alpha=0.5)
    print(x)
    plt.title('Proses GC saat Simulasi')
    plt.xlabel('waktu (ms)')
    plt.ylabel('disk-id')
    plt.yticks(range(ndisk))
    plt.xlim(5000000, 5010000)
    plt.show()

if __name__ == "__main__":
    main(sys.argv[1:])