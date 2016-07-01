set autoscale
unset log
unset label
set xtic auto
set ytic auto

set terminal qt size 1500,1000

set xlabel "insertion order"
set ylabel "insertion time [ms]"
set boxwidth 0.9
set style fill solid
set key left top
set yrange[0:*]
set style data histogram
set style fill solid 1.0 border -0.1
set xtic scale 0

set title "PH-Tree different insertion order" 
plot \
  "plot/data/phtree_insert_order.dat" using 3:xtic(1):xticlabel(2) t 'insert time'

unset output


