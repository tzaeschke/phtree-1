set autoscale
unset log
unset label
set xtic auto
set ytic auto

set xlabel "selectivity"
set terminal qt size 1500,1000

set multiplot layout 1,2 title "same dimension - same number of entries - different ranges"

set key right top
set style data histograms
set style histogram rowstacked
set boxwidth 1 relative
set style fill solid 1.0 border -1
set ylabel "total range query time [ms]"
set title "Range query"

plot \
  "plot/data/phtree_range_query_selectivity.dat" using 3 t 'initialization',\
  "" using 4:xticlabels(2) t 'range query'

set ylabel "#elements in range"
set boxwidth 0.9
set style fill solid
set key left top
set yrange[0:*]
set style data histogram
set style fill solid 1.0 border -1
set xtic scale 0

set title "Number of elements per range" 
plot \
  "plot/data/phtree_range_query_selectivity.dat" using 5:xtic(2) t '#entries in range' ls 5

unset multiplot
unset output

