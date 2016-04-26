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

#define AVERAGE_INSERT_DIM_PLOT_NAME "phtree_average_insert_dimensions"
#define AVERAGE_INSERT_ENTRIES_PLOT_NAME "phtree_average_insert_entries"
#define INSERT_SERIES_PLOT_NAME "phtree_insert_series"

#define PLOT_DATA_PATH "./plot/data/"
#define PLOT_DATA_EXTENSION ".dat"
#define GNUPLOT_FILE_PATH "./plot/"
#define GNUPLOT_FILE_EXTENSION ".p"

#define BIT_LENGTH 	32
#define ENTRY_DIM 	3
#define ENTRY_DIM_INSERT_SERIES 3

#define INSERT_ENTRY_DIMS {3, 5, 8, 11, 14, 17, 20, 23};
#define INSERT_ENTRY_NUMBERS {100000, 500000, 1000000};

#define N_REPETITIONS 10
#define N_RANDOM_ENTRIES_AVERAGE_INSERT 100000
#define N_RANDOM_ENTRIES_INSERT_SERIES 100

class Entry;

class PlotUtil {
public:
	static std::set<Entry*> generateUniqueRandomEntries(size_t dim, size_t bitLength, size_t nUniqueEntries);

	static void plotAverageInsertTimePerDimension(std::vector<std::vector<Entry*>> entries, std::vector<size_t> dimensions, std::vector<size_t> bitLengths);
	static void plotAverageInsertTimePerDimension(std::string file, size_t bitLength);
	static void plotAverageInsertTimePerDimensionRandom();

	static void plotAverageInsertTimePerNumberOfEntries(std::vector<std::vector<Entry*>> entries, std::vector<size_t> bitLengths);
	static void plotAverageInsertTimePerNumberOfEntries(std::string file, size_t bitLength);
	static void plotAverageInsertTimePerNumberOfEntriesRandom();

	static void plotTimeSeriesOfInserts();

private:
	static void plot(std::string gnuplotFileName);
	static std::ofstream* openPlotFile(std::string dataFileName);
};


#endif /* PLOTUTIL_H_ */
