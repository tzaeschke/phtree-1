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

#define INSERT_ENTRY_DIMS {3, 6, 8, 10};
#define INSERT_ENTRY_NUMBERS {100, 10000, 100000, 1000000};

#define N_REPETITIONS 10
#define N_RANDOM_ENTRIES_AVERAGE_INSERT 500000
#define N_RANDOM_ENTRIES_INSERT_SERIES 100

template <unsigned int DIM, unsigned int WIDTH>
class Entry;

class PlotUtil {
public:
	template <unsigned int DIM, unsigned int WIDTH>
	static std::set<std::vector<unsigned long>>* generateUniqueRandomEntries(size_t nUniqueEntries);

	template <unsigned int DIM, unsigned int WIDTH>
	static void writeAverageInsertTimeOfDimension(size_t runNumber, std::vector<std::vector<unsigned long>>* entries);
	template <unsigned int DIM, unsigned int WIDTH>
	static void plotAverageInsertTimePerDimension(std::string file);
	static void plotAverageInsertTimePerDimensionRandom();

	template <unsigned int DIM, unsigned int WIDTH>
	static void plotAverageInsertTimePerNumberOfEntries(std::vector<std::vector<std::vector<unsigned long>>*> entries);
	template <unsigned int DIM, unsigned int WIDTH>
	static void plotAverageInsertTimePerNumberOfEntries(std::string file);
	static void plotAverageInsertTimePerNumberOfEntriesRandom();
	static void plotAverageInsertTimePerNumberOfEntriesRandom(std::vector<size_t> nEntries);

	static void plotTimeSeriesOfInserts();

private:
	static void plot(std::string gnuplotFileName);
	static void clearPlotFile(std::string dataFileName);
	static std::ofstream* openPlotFile(std::string dataFileName, bool removePreviousData);
	template <unsigned int DIM, unsigned int WIDTH>
	static inline std::vector<std::vector<unsigned long>>* generateUniqueRandomEntriesList(size_t nUniqueEntries);
};

#include <fstream>
#include <stdlib.h>
#include <time.h>
#include <ctime>
#include <algorithm>
#include <set>
#include <stdexcept>
#include <assert.h>
#include <valgrind/callgrind.h>

#include "Entry.h"
#include "PHTree.h"
#include "visitors/CountNodeTypesVisitor.h"
#include "visitors/AssertionVisitor.h"
#include "visitors/SizeVisitor.h"
#include "visitors/PrefixSharingVisitor.h"
#include "util/FileInputUtil.h"
#include "util/rdtsc.h"

using namespace std;

template <unsigned int DIM, unsigned int WIDTH>
set<vector<unsigned long>>* PlotUtil::generateUniqueRandomEntries(size_t nUniqueEntries) {
	srand(time(NULL));
	set<vector<unsigned long>>* randomDimEntries = new set<vector<unsigned long>>();
	for (size_t nEntry = 0; nEntry < nUniqueEntries; nEntry++) {
		vector<unsigned long> entryValues(DIM);
		for (size_t d = 0; d < DIM; d++) {
			entryValues.at(d) = rand() % (1ul << WIDTH);
		}
		bool inserted = randomDimEntries->insert(entryValues).second;
		if (!inserted) {
			nEntry--;
		}
	}

	return randomDimEntries;
}

template <unsigned int DIM, unsigned int WIDTH>
std::vector<vector<unsigned long>>* PlotUtil::generateUniqueRandomEntriesList(size_t nUniqueEntries) {
	set<vector<unsigned long>>* randomDimEntriesSet = generateUniqueRandomEntries<DIM, WIDTH>(nUniqueEntries);
	vector<vector<unsigned long>>* randomDimEntries = new vector<vector<unsigned long>>(randomDimEntriesSet->begin(),
			randomDimEntriesSet->end());
	return randomDimEntries;
}

void PlotUtil::plot(string gnuplotFileName) {
	string path = GNUPLOT_FILE_PATH + gnuplotFileName + GNUPLOT_FILE_EXTENSION;
	string gnuplotCommand = "gnuplot -p '" + path + "'";
	system(gnuplotCommand.c_str());
}

template <unsigned int DIM, unsigned int WIDTH>
void PlotUtil::writeAverageInsertTimeOfDimension(size_t runNumber, vector<vector<unsigned long>>* entries)  {
		cout << "inserting all entries into a PH-Tree while logging the time per insertion..." << endl;

		PHTree<DIM, WIDTH>* phtree = new PHTree<DIM, WIDTH>();

		// clock() -> insert all entries of one dim into the appropriate tree -> clock()
		unsigned int insertTicks = 0;
		unsigned int lookupTicks = 0;
		unsigned int nAHCNodes = 0;
		unsigned int nLHCNodes = 0;
		unsigned int totalLhcBitSize = 0;
		unsigned int totalAhcBitSize = 0;
		unsigned int totalTreeBitSize = 0;
		CountNodeTypesVisitor<DIM>* visitor = new CountNodeTypesVisitor<DIM>();
		SizeVisitor<DIM>* sizeVisitor = new SizeVisitor<DIM>();
		PrefixSharingVisitor<DIM>* prefixVisitor = new PrefixSharingVisitor<DIM>();
		SuffixVisitor<DIM>* suffixVisitor = new SuffixVisitor<DIM>();

		unsigned int startInsertTime = clock();
		for (size_t iEntry = 0; iEntry < entries->size(); ++iEntry) {
			vector<unsigned long> entry = (*entries)[iEntry];
			phtree->insert(entry, iEntry);
		}
		unsigned int totalInsertTicks = clock() - startInsertTime;
		unsigned int startLookupTime = clock();
		for (size_t iEntry = 0; iEntry < entries->size(); ++iEntry) {
			vector<unsigned long> entry = (*entries)[iEntry];
			bool contained = phtree->lookup(entry).first;
			assert (contained);
		}
		unsigned int totalLookupTicks = clock() - startLookupTime;
		phtree->accept(visitor);
		phtree->accept(sizeVisitor);
		phtree->accept(prefixVisitor);
		phtree->accept(suffixVisitor);
		cout << "d=" << DIM << endl << *visitor << *prefixVisitor << *sizeVisitor << *suffixVisitor << endl;
		insertTicks = totalInsertTicks;
		lookupTicks = totalLookupTicks;
		nAHCNodes = visitor->getNumberOfVisitedAHCNodes();
		nLHCNodes = visitor->getNumberOfVisitedLHCNodes();
		totalAhcBitSize = sizeVisitor->getTotalAhcBitSize();
		totalLhcBitSize = sizeVisitor->getTotalLhcBitSize();
		totalTreeBitSize = sizeVisitor->getTotalTreeBitSize();

		// write gathered data into a file
		ofstream* plotFile = openPlotFile(AVERAGE_INSERT_DIM_PLOT_NAME, false);
		cout << "\tdim\tinsert [ms]\t\tlookup [ms]\t\tsize [bit per dimension]" << endl;
			float insertMs = (float (insertTicks) / entries->size() / (CLOCKS_PER_SEC / 1000));
			float lookupMs = (float (lookupTicks) / entries->size() / (CLOCKS_PER_SEC / 1000));
			float totalSizeBit = (float(totalAhcBitSize + totalLhcBitSize + totalTreeBitSize)) / entries->size();
			(*plotFile) << runNumber
				<< "\t" << DIM
				<< "\t"	<< insertMs
				<< "\t" << lookupMs
				<< "\t"	<< nAHCNodes
				<< "\t" << nLHCNodes
				<< "\t" << (float(totalAhcBitSize) / entries->size() / DIM)
				<< "\t" << (float(totalLhcBitSize) / entries->size() / DIM)
				<< "\t" << (float(totalTreeBitSize) / entries->size() / DIM) << "\n";
			cout << runNumber << "\t" << DIM << "\t" << insertMs << "\t\t" << lookupMs  << "\t\t" << totalSizeBit << endl;

		// clear
		delete phtree;
		delete visitor;
		delete sizeVisitor;
		delete prefixVisitor;
		delete suffixVisitor;
		plotFile->close();
		delete plotFile;
		delete entries;
}

template <unsigned int DIM, unsigned int WIDTH>
void PlotUtil::plotAverageInsertTimePerDimension(std::string file) {
	cout << "loading entries from file...";
	vector<vector<unsigned long>>* entries = FileInputUtil::readEntries<DIM, WIDTH>(file);
	cout << " ok" << endl;

	writeAverageInsertTimeOfDimension<DIM, WIDTH>(0, entries);
}

inline

void PlotUtil::plotAverageInsertTimePerDimensionRandom() {
	size_t dimTests[] = INSERT_ENTRY_DIMS
	;
	size_t dimTestsSize = sizeof(dimTests) / sizeof(*dimTests);
	clearPlotFile(AVERAGE_INSERT_DIM_PLOT_NAME);

	for (size_t test = 0; test < dimTestsSize; test++) {
		// resolve dynamic dimensions
		switch (dimTests[test]) {
		case 2: {
			vector<vector<unsigned long>>* randomDimEntries =
								generateUniqueRandomEntriesList<2, BIT_LENGTH>(N_RANDOM_ENTRIES_AVERAGE_INSERT);
						writeAverageInsertTimeOfDimension<2, BIT_LENGTH>(test, randomDimEntries);
			break;
		}
		case 3: {
			vector<vector<unsigned long>>* randomDimEntries =
								generateUniqueRandomEntriesList<3, BIT_LENGTH>(N_RANDOM_ENTRIES_AVERAGE_INSERT);
						writeAverageInsertTimeOfDimension<3, BIT_LENGTH>(test, randomDimEntries);
			break;
		}
		case 6: {
			vector<vector<unsigned long>>* randomDimEntries =
								generateUniqueRandomEntriesList<6, BIT_LENGTH>(N_RANDOM_ENTRIES_AVERAGE_INSERT);
						writeAverageInsertTimeOfDimension<6, BIT_LENGTH>(test, randomDimEntries);
			break;
		}
		case 8: {
			vector<vector<unsigned long>>* randomDimEntries =
								generateUniqueRandomEntriesList<8, BIT_LENGTH>(N_RANDOM_ENTRIES_AVERAGE_INSERT);
						writeAverageInsertTimeOfDimension<8, BIT_LENGTH>(test, randomDimEntries);
			break;
		}
		case 10: {
			vector<vector<unsigned long>>* randomDimEntries =
								generateUniqueRandomEntriesList<10, BIT_LENGTH>(N_RANDOM_ENTRIES_AVERAGE_INSERT);
						writeAverageInsertTimeOfDimension<10, BIT_LENGTH>(test, randomDimEntries);
			break;
		}
		default:
			throw std::runtime_error(
					"given dimensionality currently not supported by boilerplate code");
		}
	}

	// step 2: call Gnuplot
	cout << "calling gnuplot..." << endl;
	plot(AVERAGE_INSERT_DIM_PLOT_NAME);
}

template <unsigned int DIM, unsigned int WIDTH>
void PlotUtil::plotAverageInsertTimePerNumberOfEntries(vector<vector<vector<unsigned long>>*> entries) {
		vector<unsigned int> insertTicks(entries.size());
		vector<unsigned int> lookupTicks(entries.size());
		vector<unsigned int> nAHCNodes(entries.size());
		vector<unsigned int> nLHCNodes(entries.size());
		vector<unsigned int> totalLhcBitSize(entries.size());
		vector<unsigned int> totalAhcBitSize(entries.size());
		vector<unsigned int> totalTreeBitSize(entries.size());
		vector<unsigned int> sizes(entries.size());

		cout << "start insertions...";
		CountNodeTypesVisitor<DIM>* visitor = new CountNodeTypesVisitor<DIM>();
		SizeVisitor<DIM>* sizeVisitor = new SizeVisitor<DIM>();
		PrefixSharingVisitor<DIM>* prefixVisitor = new PrefixSharingVisitor<DIM>();
		SuffixVisitor<DIM>* suffixVisitor = new SuffixVisitor<DIM>();

		for (size_t test = 0; test < entries.size(); test++) {
			PHTree<DIM, WIDTH>* tree = new PHTree<DIM, WIDTH>();
			const unsigned int startInsertTime = clock();
			for (size_t iEntry = 0; iEntry < entries[test]->size(); iEntry++) {
				vector<unsigned long> entry = (*entries[test])[iEntry];
				tree->insert(entry, iEntry);
			}
			const unsigned int totalInsertTicks = clock() - startInsertTime;
			CALLGRIND_START_INSTRUMENTATION;
			const unsigned int startLookupTime = clock();
			for (size_t iEntry = 0; iEntry < entries[test]->size(); iEntry++) {
				vector<unsigned long> entry = (*entries[test])[iEntry];
				tree->lookup(entry);
			}
			const unsigned int totalLookupTicks = clock() - startLookupTime;
			CALLGRIND_STOP_INSTRUMENTATION;
			tree->accept(visitor);
			tree->accept(sizeVisitor);
			tree->accept(prefixVisitor);
			tree->accept(suffixVisitor);
			cout << "n=" << entries[test]->size() << endl << *visitor << *prefixVisitor << *sizeVisitor << *suffixVisitor << endl;

			insertTicks.at(test) = totalInsertTicks;
			lookupTicks.at(test) = totalLookupTicks;
			nAHCNodes.at(test) = visitor->getNumberOfVisitedAHCNodes();
			nLHCNodes.at(test) = visitor->getNumberOfVisitedLHCNodes();
			totalLhcBitSize.at(test) = sizeVisitor->getTotalLhcBitSize();
			totalAhcBitSize.at(test) = sizeVisitor->getTotalAhcBitSize();
			totalTreeBitSize.at(test) = sizeVisitor->getTotalTreeBitSize();
			sizes.at(test) = entries[test]->size();

			visitor->reset();
			sizeVisitor->reset();
			prefixVisitor->reset();
			suffixVisitor->reset();
			delete tree;
			delete entries[test];
		}
		delete visitor;
		delete sizeVisitor;
		delete prefixVisitor;
		delete suffixVisitor;

		cout << " ok" << endl;
		// write gathered data into a file
		ofstream* plotFile = openPlotFile(AVERAGE_INSERT_ENTRIES_PLOT_NAME, true);
		for (size_t test = 0; test < entries.size(); test++) {
			(*plotFile) << test << "\t"
					<< sizes[test] << "\t"
					<< (float (insertTicks[test]) / sizes[test] / CLOCKS_PER_SEC * 1000) << "\t"
					<< (float (lookupTicks[test]) / sizes[test] / CLOCKS_PER_SEC * 1000) << "\t"
					<< nAHCNodes.at(test) << "\t"
					<< nLHCNodes.at(test) << "\t"
					<< (float(totalAhcBitSize.at(test)) / sizes[test] / ENTRY_DIM_INSERT_SERIES) << "\t"
					<< (float(totalLhcBitSize.at(test)) / sizes[test] / ENTRY_DIM_INSERT_SERIES) << "\t"
					<< (float(totalTreeBitSize.at(test)) / sizes[test] / ENTRY_DIM_INSERT_SERIES) << "\n";
		}
		plotFile->close();
		delete plotFile;

		// step 2: call Gnuplot
		cout << "calling gnuplot...";
		plot(AVERAGE_INSERT_ENTRIES_PLOT_NAME);
		cout << " ok" << endl;
}

template <unsigned int DIM, unsigned int WIDTH>
void PlotUtil::plotAverageInsertTimePerNumberOfEntries(std::string file) {
	vector<Entry<DIM, WIDTH>*> entries = FileInputUtil::readEntries<DIM, WIDTH>(file);
	vector<vector<Entry<DIM, WIDTH>*>> singleColumnEntries(1);
	singleColumnEntries.at(0) = entries;

	plotAverageInsertTimePerNumberOfEntries<DIM>(singleColumnEntries);
}

void PlotUtil::plotAverageInsertTimePerNumberOfEntriesRandom() {
	size_t numberOfEntries[] = INSERT_ENTRY_NUMBERS;
	size_t numberOfEntriesSize = sizeof(numberOfEntries) / sizeof(*numberOfEntries);
	vector<size_t> nEntries(numberOfEntries, numberOfEntries + numberOfEntriesSize);

	plotAverageInsertTimePerNumberOfEntriesRandom(nEntries);
}

void PlotUtil::plotAverageInsertTimePerNumberOfEntriesRandom(vector<size_t> nEntries) {
	vector<vector<vector<unsigned long>>*> testEntries(nEntries.size());
	for (unsigned test = 0; test < nEntries.size(); ++test) {
		testEntries.at(test) = generateUniqueRandomEntriesList<ENTRY_DIM, BIT_LENGTH>(nEntries.at(test));
	}

	plotAverageInsertTimePerNumberOfEntries<ENTRY_DIM, BIT_LENGTH>(testEntries);
}

void PlotUtil::clearPlotFile(std::string dataFileName) {
	std::string path = PLOT_DATA_PATH + dataFileName + PLOT_DATA_EXTENSION;
	ofstream* plotFile = new ofstream();
	plotFile->open(path.c_str(), ofstream::out | ofstream::trunc);
	plotFile->close();
	delete plotFile;

}

ofstream* PlotUtil::openPlotFile(std::string dataFileName, bool removePreviousData) {
	std::string path = PLOT_DATA_PATH + dataFileName + PLOT_DATA_EXTENSION;
	ofstream* plotFile = new ofstream();
	if (removePreviousData) {
		plotFile->open(path.c_str(), ofstream::out | ofstream::trunc);
	} else {
		plotFile->open(path.c_str(), ofstream::out | ofstream::app);
	}
	return plotFile;
}

void PlotUtil::plotTimeSeriesOfInserts() {
	set<vector<unsigned long>>* entries = generateUniqueRandomEntries<ENTRY_DIM, BIT_LENGTH>(N_RANDOM_ENTRIES_INSERT_SERIES);
	PHTree<ENTRY_DIM, BIT_LENGTH> phtree;
	ofstream* plotFile = openPlotFile(INSERT_SERIES_PLOT_NAME, true);

	CountNodeTypesVisitor<ENTRY_DIM>* visitor = new CountNodeTypesVisitor<ENTRY_DIM>();
	AssertionVisitor<ENTRY_DIM>* assertVisitor = new AssertionVisitor<ENTRY_DIM>();
	SizeVisitor<ENTRY_DIM>* sizeVisitor = new SizeVisitor<ENTRY_DIM>();
	try {
		size_t iEntry = 0;
		for (auto entry : (*entries)) {
//			cout << "inserting: " << (&entry) << endl;
			assert (!phtree.lookup(entry).first && "should not contain the entry before insertion");
			uint64_t startInsert = RDTSC();
			phtree.insert(entry, iEntry);
			uint64_t totalInsertTicks = RDTSC() - startInsert;
//			cout << phtree << endl;
			phtree.accept(assertVisitor);
			phtree.accept(visitor);
			phtree.accept(sizeVisitor);
			uint64_t startLookup = RDTSC();
			pair<bool, int> contained = phtree.lookup(entry);
			uint64_t totalLookupTicks = RDTSC() - startLookup;
			assert (contained.first && "should contain the entry after insertion");
			(*plotFile) << iEntry << "\t" << totalInsertTicks;
			(*plotFile) << "\t" << totalLookupTicks;
			(*plotFile) << "\t" << visitor->getNumberOfVisitedAHCNodes();
			(*plotFile) << "\t" << visitor->getNumberOfVisitedLHCNodes();
			(*plotFile) << "\t" << sizeVisitor->getTotalAhcByteSize();
			(*plotFile) << "\t" << sizeVisitor->getTotalLhcByteSize();
			(*plotFile) << "\n";
			assertVisitor->reset();
			visitor->reset();
			sizeVisitor->reset();
			plotFile->flush();
			iEntry++;
		}
	} catch (const exception& e) {
		cout << e.what();
	}

	size_t iEntry = 0;
	for (auto entry : (*entries)) {
		pair<bool, int> contained = phtree.lookup(entry);
		assert (contained.first && contained.second == iEntry);
		iEntry++;
	}

	plotFile->close();
	delete plotFile;
	delete visitor;
	delete assertVisitor;
	delete sizeVisitor;
	delete entries;

	plot(INSERT_SERIES_PLOT_NAME);
}

#endif /* PLOTUTIL_H_ */
