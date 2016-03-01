set autoscale
unset log
unset label
set xrange[0:*]
set xtic auto
set ytic auto
set ylabel "insert time [ms]"
set xlabel "#inserted entries"
set key right bottom

set terminal wxt size 1300,1000

set title "PH-Tree insert performance" 
plot \
  "plot/data/phtree.dat" using 1:2 title 'dim 2' with lines,\
  "plot/data/phtree.dat" using 1:3 title 'dim 5' with lines,\
  "plot/data/phtree.dat" using 1:4 title 'dim 20' with lines
  
unset output

