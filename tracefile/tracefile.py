import numpy as np
import matplotlib.pyplot as plt
import sys

tracefile = open(sys.argv[1], 'r')

nrequests = 0
nreadrequest = 0
req_time = []
req_size_r = []
req_size_w = []

# gathering data
for line in tracefile:
    incoming_time = float(line.split()[0])
    disk_id = line.split()[1]
    address = line.split()[2]
    size = float(line.split()[3])
    ope = line.split()[4]

    nrequests = nrequests + 1
    req_time.append(incoming_time)
    if ope == "0":
        req_size_r.append(size/2.0)
        nreadrequest = nreadrequest + 1
    else:
        req_size_w.append(size/2.0)

diff = []
for i in range(len(req_time)-1):
    diff.append(req_time[i+1]-req_time[i])

np_diff = np.array(diff)    
np_size_r = np.array(req_size_r)
np_size_w = np.array(req_size_w)

print("int-arrival time ", np.average(np_diff)/1000000.0, " ms")
print("avg.read size ", np.average(np_size_r), " KB")
print("avg.write size ", np.average(np_size_w), " KB")
print("read (%) ", nreadrequest/nrequests*100.0 , "%")

# creating io rate graph
np_time = np.array(req_time)
iorate,_ = np.histogram(np_time, bins=1000)
plt.plot(iorate)
plt.ylim(top=3000)
plt.show()
