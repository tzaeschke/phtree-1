set autoscale
unset log
unset label

set terminal qt size 1300,1000
set multiplot layout 1,3 title "parallel insertion"

set xrange[1:*]
set xtic 1
set ytic auto
set xlabel "#threads"

set key left top

set ylabel "total insertion time [ms]"
set title "absolute insertion time" 
plot \
  "plot/data/phtree_parallel_insert.dat" using 1:2 with linespoints pt 7 ps 1

set ylabel "Million insertions per second"
set title "Throughput" 
plot \
  "plot/data/phtree_parallel_insert.dat" using 1:3 with linespoints pt 7 ps 1

set yrange[0:1]
set ytic 0.1
set ylabel "T_{seq} / T_{#threads}"
set title "efficiency (compared to sequential insertion)" 
plot \
  "plot/data/phtree_parallel_insert.dat" using 1:4 with linespoints pt 7 ps 1

unset multiplot
unset output


