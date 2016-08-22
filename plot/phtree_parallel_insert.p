set autoscale
unset log
unset label

set colorsequence default
set terminal qt size 1300,1000
set multiplot layout 1,3 title "parallel insertion"

set xrange[1:*]
set xtic 1
set ytic auto
set xlabel "#threads"

set key left top

set ylabel "total insertion time [ms]"
set yrange[0:*]
set title "absolute insertion time" 
plot \
  "plot/data/phtree_parallel_insert.dat" every 4::0 using 1:3 with linespoints pt 6 ps 1 lc 1 t 'optimistic - range per thread',\
  "plot/data/phtree_parallel_insert.dat" every 4::1 using 1:3 with linespoints pt 6 ps 1 lc 2 t 'buffered - range per thread',\
  "plot/data/phtree_parallel_insert.dat" every 4::2 using 1:3 with linespoints pt 7 ps 1 dashtype 2 lc 1 t 'optimistic - atomic range index',\
  "plot/data/phtree_parallel_insert.dat" every 4::3 using 1:3 with linespoints pt 7 ps 1 dashtype 2 lc 2 t 'buffered - atomic range index'

set ylabel "Million insertions per second"
unset yrange
set title "Throughput" 
plot \
  "plot/data/phtree_parallel_insert.dat" every 4::0 using 1:4 with linespoints pt 6 ps 1 lc 1 t 'optimistic - range per thread',\
  "plot/data/phtree_parallel_insert.dat" every 4::1 using 1:4 with linespoints pt 6 ps 1 lc 2 t 'buffered - range per thread',\
  "plot/data/phtree_parallel_insert.dat" every 4::2 using 1:4 with linespoints pt 7 ps 1 dashtype 2 lc 1 t 'optimistic - atomic range index',\
  "plot/data/phtree_parallel_insert.dat" every 4::3 using 1:4 with linespoints pt 7 ps 1 dashtype 2 lc 2 t 'buffered - atomic range index'

set yrange[0:1]
set ytic 0.1
set ylabel "T_{seq} / T_{#threads}"
set title "efficiency (compared to sequential insertion)" 
plot \
  "plot/data/phtree_parallel_insert.dat" every 4::0 using 1:5 with linespoints pt 6 ps 1 lc 1 t 'optimistic - range per thread',\
  "plot/data/phtree_parallel_insert.dat" every 4::1 using 1:5 with linespoints pt 6 ps 1 lc 2 t 'buffered - range per thread',\
  "plot/data/phtree_parallel_insert.dat" every 4::2 using 1:5 with linespoints pt 7 ps 1 dashtype 2 lc 1 t 'optimistic - atomic range index',\
  "plot/data/phtree_parallel_insert.dat" every 4::3 using 1:5 with linespoints pt 7 ps 1 dashtype 2 lc 2 t 'buffered - atomic range index'

unset multiplot
unset output

