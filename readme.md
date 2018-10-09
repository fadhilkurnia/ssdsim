### Statement:
SSDsim is a simulation tool of SSDs internal hardware and software behavior. It provides specified SSDs performance, endurance and energy consumption information based on a configurable parameter file and different workloads (trace file).
SSDsim was created by Yang Hu in the end of 2009 and upgraded to version 2.0 after lots of modification and perfection. Its programming language is C and development environment is Microsoft Visual Studio 2008. With the help of Zhiming Zhu, Shuangwu Zhang, Chao Ren, Hao Luo, it is further developed into version 2.x. As the development team, we will continue adding new modules and functions to guarantee its persistent perfection. If you have any questions, suggestions or requirements about it, please feel free to email Yang Hu (yanghu@foxmail.com). We will adopt any reasonable requirements to make SSDsim better.

### Application steps:
1)	Compile the code and generate an executablefile.
```
$ make clean
```
```
$ make all
```
2)	Run the executable file (`ssd`), followed by trace file name, it's required.
```
$ ./ssd your_trace_file.trace
```
3)	Analyze the output file and statistic file for further experiments.

### Note:
1)	Each request of the trace file possesses one line. A request should be ASCII and follow the format: request arriving time(int64), logical device number (int), logical sector number (int), request size (int), access operator (int, 1 for read, 0 for write). 
2)	The unit of the request arriving time is nanosecond and the unit of the request size is sector (512 Bytes).
3)	No CRLF at the end of the last request line.


2018-06-05

___

### How To Run Simulation and Generate CDF Graph

1) Select SSD configuration that you want to use in your simulation. All predefined config can be seen at `config` directory. For example, to run SSD simulation with 1 channel, use this command:
```
$ mv config/1channel.conf page.parameters
```

2) Run the simulation and tracefile that you want to use, dont forget to compile SSDSim first.
```
$ ./ssd your_trace_file.trace
```

3) Your simulation statistics and log will be created at raw/`timestamp`/ directory (e.g raw/20181010_110000/). Each simulation will have its own directory to store all the statistics.

4) Generate the cdf graph using processing/cdf script. That script need your latency data from the simulation, for example:
```
$ python processing/cdf raw/20181010_110000/io_read.dat
```

5) Your cdf graph can be seen at processing/cdf_eps/ directory