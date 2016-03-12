set autoscale
unset log
unset label
set xtic auto
set ytic auto

set xlabel "entry dimension"
set terminal wxt size 1500,1000

set multiplot layout 1,2 title "various dimensions - same number of insertions"

set ylabel "insertion time [clock ticks]"
set boxwidth 0.5
set style fill solid
set key left top
set yrange[0:*]

set title "PH-Tree insertion" 
plot \
  "plot/data/phtree_average_insert.dat" using 1:3:xtic(2) title 'insert time' with boxes ls 1

set key right top
set style data histograms
set style histogram rowstacked
set boxwidth 1 relative
set style fill solid 1.0 border -1
set ylabel "number of nodes"
set title "PH-Tree nodes"

plot \
  "plot/data/phtree_average_insert.dat" using 4 t '#AHC' ls 2,\
  "" using 5:xticlabels(2) t '#LHC' ls 3

unset multiplot
unset output

