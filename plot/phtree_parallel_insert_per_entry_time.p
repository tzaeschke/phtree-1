set autoscale
unset log
unset label

set xrange[0:*]
set yrange[0:*]
set xtic auto
set ytic auto
set ylabel "time [nanos]"
set xlabel "inserted entry number"
set key left top

set terminal qt size 1300,1000

n = 1
dat = sprintf("plot/data/timeseries-%d.dat", n)
f(x) = sprintf("thread %d", x)

set title "PH-Tree insert series" 
plot \
  for [i=1:n] dat using 1:i+1 title f(i) with linespoints


unset output

