/*
 * PlotUtil.cpp
 *
 *  Created on: Mar 3, 2016
 *      Author: max
 */

#include <fstream>
#include <stdlib.h>
#include <time.h>
#include <ctime>
#include <algorithm>
#include <assert.h>

#include "PlotUtil.h"
#include "Entry.h"
#include "PHTree.h"

using namespace std;

vector<Entry*> PlotUtil::generateUniqueRandomEntries(size_t dim, size_t bitLength, size_t nUniqueEntries) {
	vector<Entry*> randomDimEntries;
	for (size_t nEntry = 0; nEntry < nUniqueEntries; nEntry++) {
		vector<int>* entryValues = new vector<int>(dim);
		for (size_t d = 0; d < dim; d++) {
			entryValues->at(d) = rand() % (2 << bitLength);
		}
		Entry* entry = new Entry(*entryValues, bitLength);

		if (find(randomDimEntries.begin(), randomDimEntries.end(), entry)
				!= randomDimEntries.end()) {
			// duplicate
			nEntry--;
		} else {
			randomDimEntries.push_back(entry);
		}
	}

	return randomDimEntries;
}

void PlotUtil::plot(string gnuplotFileName) {
	string path = GNUPLOT_FILE_PATH + gnuplotFileName + GNUPLOT_FILE_EXTENSION;
	string gnuplotCommand = "gnuplot -p '" + path + "'";
	system(gnuplotCommand.c_str());
}


void PlotUtil::plotAverageInsertTimePerDimension() {

	srand(time(NULL));

	// step 1: gather phtree data
	// 1.1. create X random entries for Y different dimensions
	cout << "Creating " << N_RANDOM_ENTRIES_AVERAGE_INSERT	<< " random entries per dimension..." << endl;
	size_t dimTests[] = ENTRY_DIMS;
	size_t dimTestsSize = sizeof(dimTests) / sizeof(*dimTests);
	vector<vector<Entry*>> randomEntries;
	for (size_t test = 0; test < dimTestsSize; test++) {
		vector<Entry*> randomDimEntries = generateUniqueRandomEntries(dimTests[test], BIT_LENGTH, N_RANDOM_ENTRIES_AVERAGE_INSERT);
		randomEntries.push_back(randomDimEntries);
	}

	// 1.2. add all entries to the PH-Tree and measure the time per insertion
	cout << "inserting all entries into a PH-Tree while logging the time per insertion..." << endl;

	vector<PHTree*> phtrees;
	for (size_t test = 0; test < dimTestsSize; test++) {
		PHTree* tree = new PHTree(dimTests[test], BIT_LENGTH);
		phtrees.push_back(tree);
	}

	// clock() -> insert all entries of one dim into the appropriate tree -> clock()
	vector<unsigned int> insertMillis(N_RANDOM_ENTRIES_AVERAGE_INSERT);
	for (size_t test = 0; test < dimTestsSize; test++) {
		unsigned int startTime = clock();
		for (size_t iEntry = 0; iEntry < N_RANDOM_ENTRIES_AVERAGE_INSERT; iEntry++) {
			Entry* entry = randomEntries[test][iEntry];
			phtrees[test]->insert(entry);
		}
		unsigned int totalMillis = clock() - startTime;
		insertMillis.at(test) = totalMillis;
	}

	// write gathered data into a file
	ofstream* plotFile = openPlotFile(AVERAGE_INSERT_PLOT_NAME);
	for (size_t test = 0; test < dimTestsSize; test++) {
		(*plotFile) << dimTests[test] << "\t" << (insertMillis[test] / N_RANDOM_ENTRIES_AVERAGE_INSERT) << "\n";
	}
	plotFile->close();

	// step 2: call Gnuplot
	cout << "calling gnuplot..." << endl;
	plot(AVERAGE_INSERT_PLOT_NAME);
};

ofstream* PlotUtil::openPlotFile(std::string dataFileName) {
	std::string path = PLOT_DATA_PATH + dataFileName + PLOT_DATA_EXTENSION;
	ofstream* plotFile = new ofstream();
	plotFile->open(path.c_str(), ofstream::out | ofstream::trunc);
	return plotFile;
}

void PlotUtil::plotTimeSeriesOfInserts() {
	vector<Entry*> entries = generateUniqueRandomEntries(ENTRY_DIM, BIT_LENGTH, N_RANDOM_ENTRIES_INSERT_SERIES);
	PHTree phtree(ENTRY_DIM, BIT_LENGTH);
	ofstream* plotFile = openPlotFile(INSERT_SERIES_PLOT_NAME);

	for (size_t iEntry = 0; iEntry < N_RANDOM_ENTRIES_AVERAGE_INSERT; iEntry++) {
		Entry* entry = entries[iEntry];
		unsigned int startTime = clock();
		phtree.insert(entry);
		unsigned int totalMillis = clock() - startTime;
		assert (phtree.lookup(entry));
		(*plotFile) << iEntry << "\t" << totalMillis;
	}

	plotFile->close();
	plot(INSERT_SERIES_PLOT_NAME);
}
