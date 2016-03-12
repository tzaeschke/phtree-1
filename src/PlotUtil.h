/*
 * PlotUtil.h
 *
 *  Created on: Mar 7, 2016
 *      Author: max
 */

#ifndef PLOTUTIL_H_
#define PLOTUTIL_H_

#include <string>
#include <vector>
#include <set>
#include <iostream>

#define AVERAGE_INSERT_PLOT_NAME "phtree_average_insert"
#define INSERT_SERIES_PLOT_NAME "phtree_insert_series"

#define PLOT_DATA_PATH "./plot/data/"
#define PLOT_DATA_EXTENSION ".dat"
#define GNUPLOT_FILE_PATH "./plot/"
#define GNUPLOT_FILE_EXTENSION ".p"

#define BIT_LENGTH 	8
#define ENTRY_DIM 	2
#define ENTRY_DIMS {2, 5, 8, 11, 14, 17, 20};

#define N_REPETITIONS 10
#define N_RANDOM_ENTRIES_AVERAGE_INSERT 90
#define N_RANDOM_ENTRIES_INSERT_SERIES 80

class Entry;

class PlotUtil {
public:
	static std::set<Entry*> generateUniqueRandomEntries(size_t dim, size_t bitLength, size_t nUniqueEntries);
	static void plotAverageInsertTimePerDimension();
	static void plotTimeSeriesOfInserts();

private:
	static void plot(std::string gnuplotFileName);
	static std::ofstream* openPlotFile(std::string dataFileName);
};


#endif /* PLOTUTIL_H_ */
