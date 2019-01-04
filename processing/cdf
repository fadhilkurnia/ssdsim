#!/usr/bin/env python3
import sys, argparse, os

def main(argv):
    GCRT = False
    parser = argparse.ArgumentParser(description='cumulative distribution function')
    parser.add_argument('infile', type=argparse.FileType('r'), help="location of the io log input file (generated from ssdsim simulation)")
    parser.add_argument('-gcrt', action='store_true', help="create the cdf for gc remaining time (default: io latency)")
    args = parser.parse_args()

    rootDir = os.path.dirname(os.path.realpath(__file__))
    if not os.path.exists(rootDir + '/cdf_data'):
        os.mkdir(rootDir + '/cdf_data')

    raw_infile = args.infile.name.split('/')[1]
    outfilename = rootDir + '/cdf_data/' + str(raw_infile) + '.dat'
    if args.gcrt:
        outfilename = rootDir + '/cdf_data/' + str(raw_infile) + '_gcrt.dat'
    outfile = open(outfilename, 'w')
    
    if args.gcrt:
        prepareGNUPlotInput(args.infile, outfile, 'gcrt')
    else:
        prepareGNUPlotInput(args.infile, outfile, 'io')

    args.infile.close()
    outfile.close()

    if not os.path.exists(rootDir + '/cdf_eps'):
        os.mkdir(rootDir + '/cdf_eps')

    data_filename = outfilename
    eps_filename = rootDir + '/cdf_eps/' + raw_infile + '.eps'
    if args.gcrt:
        eps_filename = rootDir + '/cdf_eps/' + raw_infile + '_gcrt.eps'

    graph_title = 'SSD IO Read Latency CDF Graph'
    graph_xlabel = 'Latency (ms)'
    if args.gcrt:
        graph_title = 'SSD GC Remaining Time for Each IO CDF Graph'
        graph_xlabel = 'GC Remaining Time (ms)'

    command = 'gnuplot -p -c ' + rootDir + '/cdf.plt ' + data_filename + ' ' + eps_filename + ' "'+ graph_title +'" "'+ graph_xlabel +'"'
    # print(command)
    os.system(command)
    print('\tView the generated CDF graph at ' + eps_filename + '\n\n')

def prepareGNUPlotInput(infile, outfile, cdf_type = 'io'):
    data = []
    if cdf_type == 'io':
        for line in infile:
            latency = float(line.split()[6])
            data.append(latency)
    elif cdf_type == 'gcrt': # GC remaining time
        for line in infile:
            gc_remaining_time = float(line.split()[8])
            data.append(gc_remaining_time)

    data.sort()
    
    for i, line in enumerate(data, 1):
        outfile.write("%.3f\t%.6f\n"%(line, float(i)/len(data))) 

    return data

    
if __name__ == "__main__":
    main(sys.argv[1:])
