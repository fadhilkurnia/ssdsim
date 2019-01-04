set term postscript eps color 20
set output ARG2
set title ARG3
set xlabel ARG4
set yrange [0.93 : 1]
set ylabel "CDF (Percentile)"
# set xrange [-1 : 80]
# set xtics 20
set ytics 0.01
set ytics ("1"   1,    ".99" 0.99, ".98" 0.98, ".97" 0.97, ".96" 0.96, \
           ".95" 0.95, ".94" 0.94, ".93" 0.93, ".92" 0.92, ".91" 0.91, \
           ".90" 0.90, ".89" 0.89, ".88" 0.88, ".87" 0.87, ".86" 0.86, \
           ".85" 0.85, ".84" 0.84, ".83" 0.83, ".82" 0.82, ".81" 0.81)

set tics out

plot ARG1 u ($1/1E6):2 w lines lw 8 lt 1 lc rgb "blue"  not