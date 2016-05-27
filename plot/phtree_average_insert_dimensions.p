set autoscale
unset log
unset label
set xtic auto
set ytic auto

set xlabel "entry dimension"
set terminal qt size 1500,1000

set multiplot layout 1,3 title "various dimensions - same number of operations - same number of entries"

set ylabel "insertion time [ms]"
set boxwidth 0.9
set style fill solid
set key left top
set yrange[0:*]
set style data histogram
set style fill solid 1.0 border -1
set xtic scale 0


set title "PH-Tree operations" 
plot \
  "plot/data/phtree_average_insert_dimensions.dat" using 3:xtic(2) t 'insert time',\
  "" using 4 t 'lookup time',\
  "" using 5 t 'range query time' ls 4

set key right top
set style data histograms
set style histogram rowstacked
set boxwidth 1 relative
set style fill solid 1.0 border -1
set ylabel "number of nodes"
set title "PH-Tree nodes"

plot \
  "plot/data/phtree_average_insert_dimensions.dat" using 6 t '#AHC' ls 3,\
  "" using 7:xticlabels(2) t '#LHC' ls 5

set ylabel "bits per inserted entry per dimension"
set title "PH-Tree in-memory size"

plot \
  "plot/data/phtree_average_insert_dimensions.dat" using 8 t 'AHC size' ls 3,\
  "" using 9:xticlabels(2) t 'LHC size' ls 5,\
  "" using 10:xticlabels(2) t 'suffix blocks size' ls 6

unset multiplot
unset output

