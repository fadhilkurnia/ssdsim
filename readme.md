# SSDSim
Trace based SSD simulator.

#### Statement:
SSDsim is a simulation tool of SSDs internal hardware and software behavior. It provides specified SSDs performance, endurance and energy consumption information based on a configurable parameter file and different workloads (trace file).
SSDsim was created by Yang Hu in the end of 2009 and upgraded to version 2.0 after lots of modification and perfection. Its programming language is C and development environment is Microsoft Visual Studio 2008. With the help of Zhiming Zhu, Shuangwu Zhang, Chao Ren, Hao Luo, it is further developed into version 2.x. As the development team, we will continue adding new modules and functions to guarantee its persistent perfection. If you have any questions, suggestions or requirements about it, please feel free to email Yang Hu (yanghu@foxmail.com). We will adopt any reasonable requirements to make SSDsim better.

forked from: https://github.com/huaicheng/ssdsim

______
## List of contents:
- [How to Compile](#how-to-compile)
- [How to Run the Simulation](#how-to-run-the-simulation)
- [Run Passive RAID Simulation (Unmaintained)](#run-passive-raid-simulation-unmaintained)
- [Run RAID Simulation](#run-raid-simulation)
- [Further Processing After Simulation](#further-processing-after-simulation)
- [Modifying the Tracefile](#modifying-the-tracefile)
______

## How to Compile
We already prepared the makefile. In unix environment, the program can be compiled by:
```
make all
```
To remove the previously compiled program, use:
```
make clean
```
If the compilation is success, there will be an executable file named `ssd`.


## How to Run the Simulation

SSDSim is trace-based SSD simulator, to start the simulation we need to prepare the tracefile first. SSDSim also need a configuration file to config the SSD, the default configuration file is `page.parameters`, another configuration file can be found at `config` directory.

1)	Make sure you have already compile the program (`ssd`)
2)  Prepare the tracefile in `tracefile` directory, and also the configuration as `page.parameters` file.

   We already provide some configurations that can be used, such as `1channel.conf` to simulate a SSD with only a single channel, or `8channel.conf` for a SSD with 8 channels. To use that configuration just copy that file to `page.paramaters`, for example:

```
mv config/1channel.conf page.parameters
```

3)	Run the executable file (`ssd`), followed by trace file name, it's required. Some of the tracefiles (TPCC, DTRS, etc) can be accessed at `tracefile` directory.
```
./ssd <your_trace_file.trace>

example:
./ssd tracefile/TPCC.trace
```
3)	Analyze the output file and statistic file for further experiments.

A simulation will generate a few statistic files in `raw` directory, inside the directory named the timestamp when the simulation starts (e.g :`raw/20181219_180000`). Each simulation will have its own directory to store all the statistics. Some of the files are :
- `gc.dat` contains all the gc happen during the simulation
- `io.dat` contains all the io request processed
- `io_read.dat` contains all the read io request processed
- `io_write.dat` contains all the write io request processed

### Note:
1)	Each request of the trace file possesses one line. A request should be ASCII and follow the format: request arriving time(int64), logical device number (int), logical sector number (int), request size (int), access operator (int, 1 for read, 0 for write). 
2)	The unit of the request arriving time is nanosecond and the unit of the request size is sector (512 Bytes).


## Run Passive RAID Simulation (Unmaintained)
We have add a script named `raid5` to simulate RAID SSD, currently we only support RAID5. The script receive the number of disk on the RAID configuration, and also the tracefile used for the simulation. Make sure you have already add executable permission to `raid5`, you can do that by:
```
sudo chmod +x raid5
```

To run the simulation:
```
./raid5 <ndisk> <tracefile/your_trace_file.trace> [--gcsync] [--gcsync_time_window GCSYNC_TIME_WINDOW]

example:
./raid5 4 tracefile/TPCC.trace

# to running the simulation with GCSync mode
./raid5 4 tracefile/TPCC.trace --gcsync --gcsync_time_window 500000000
```

The script will split the given tracefile into multiple tracefiles, each for one SSD in the RAID simulation. After splitting the tracefile, the script will run the simulation in each SSD using previously generated tracefile for that SSD. All the directory containing the statistics from each SSD can be seen at the log file generated from this script. (e.g `raw/raid_20181219_180000`)


## Run RAID Simulation
We have add `raid.h` module in SSDSim, so now it can simulate RAID too. It supports RAID0 and RAID5 simulation. To run RAID5 simulation you can use this command:
```
./ssd --raid5 --ndisk <number_of_disk> <your_tracefile.trace>

example:
./ssd --raid5 --ndisk 4 tracefile/TPCC.trace
```

Using this `raid` module, the simulator won't split the tracefile, it split the each request directly during the simulation. That is handled at `raid_distribute_request()` function. This RAID simulation produces a single file in `raw` directory (e.g `raw/raid_20190601_123400.log`). It cointains list of directory where the log files of each SSD, in the simulated RAID, stored.

GC Scheduling Mode:
- GCSync
```
./ssd --raid5 --ndisk <number_of_disk> --gcsync --gc_time_window <time_window> <your_tracefile.trace>
```
- GCSync+

Just the same with GCsync, but with additional buffer time between two time window, Set the buffer time at `GCSSYNC_BUFFER_TIME` constant in `initialize.h`, the default value is 0.

- GCLock
```
./ssd --raid5 --ndisk <number_of_disk> --gclock <your_tracefile.trace>
```

___
## Further Processing After Simulation
We have provided some python3 scripts that you can use to process the log generated from each simulation. You can access the script at `processing` directory, here is the description of them:

| Script | Description | Work for |
|-----|-----|-----|
| `gc_collision` | Plot the number of GC collision per read request from RAID simulation. nGC means the number of read request that meet n GC process at different disk.| RAID |
|`cdf_single.py` | Plot the CDF of I/O latency from single SSD simulation. | SSD |
| `cdf_raid` | Plot the CDF of I/O latency from RAID simulation. | RAID |
| `gc_raid.py` | Show the GC process in the middle of RAID simulation using gantt-chart-like graph | RAID |
| `fb_raid.py` | Plot the percentage of free-block per second throughout RAID simulation | RAID |
| `de_time.py` | Plot the number of block direct erase per second during GC process throughout RAID simulation | RAID |
| `gc_time.py` | Plot the number of GC per second throughout RAID simulation | RAID |
| `mp_time.py` | Plot the number of valid-page moved during GC per second throughout RAID simulation | RAID |

## Modifying the Tracefile
We also provided some python3 scripts to process a tracefile, here is the description of them:

| Script | Description |
|-----|-----|
| `tracefile.py` | Get the characteristic of a tracefile, and plot its IO rate |
| `warmup.py` | Make a new tracefile that start with warmup workload. The warmup workload is sequential full 128kb write and random 4kb write followed by the real tracefile|
| `trace-concater.py` | Make a longer tracefile by concatenate the given tracefile n times |
| `tracefile-intensive.py` | Make a more or less intensive tracefile by multiply the incoming time with x|


___
Last update: 2019-06-13