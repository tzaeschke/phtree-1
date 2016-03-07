set autoscale
unset log
unset label
set xrange[0:*]
set xtic auto
set ytic auto
set ylabel "average insert time [ms]"
set xlabel "dimension"
set key right bottom

set terminal wxt size 1300,1000

set title "PH-Tree insert performance" 
plot \
  "plot/data/phtree.dat" using 1:2 title 'total insert performance' with linespoints
  
unset output

