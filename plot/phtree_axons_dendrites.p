set autoscale
unset log
unset label
set xtic auto
set ytic auto

set xlabel "Axons and Dendrites (range queries)"
set terminal qt size 1500,1000

set multiplot layout 1,2 title "Axons and Dendrites (range queries - 6D)"

set key right top

set ylabel "total insertion time [sec]"
set boxwidth 0.9
set style fill solid
set key left top
set yrange[0:*]
set style data histogram
set style fill solid 1.0 border -1
set xtic scale 0

set title "Intersection of Axons and Dendrites" 
plot \
  "plot/data/phtree_axons_dendrites.dat" using 5:xticlabels(3) t 'insertion' ls 5

set style data histograms
set style histogram rowstacked
set boxwidth 1 relative
set style fill solid 1.0 border -1
set ylabel "total query time [sec]"
set title "Range queries using Axons"

plot \
  "plot/data/phtree_axons_dendrites.dat" using 6 t 'initialization' ls 3,\
  "" using 7 t 'range query' ls 6,\
  "" using 5:xticlabels(2) t 'insertion' ls 5 fillstyle pattern 4

unset multiplot
unset output

