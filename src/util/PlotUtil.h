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

#define INSERT_ENTRY_DIMS {3, 5};
#define INSERT_ENTRY_NUMBERS {1000, 10000};

#define N_REPETITIONS 10
#define N_RANDOM_ENTRIES_AVERAGE_INSERT 1000
#define N_RANDOM_ENTRIES_INSERT_SERIES 100

template <unsigned int DIM, unsigned int WIDTH>
class Entry;

class PlotUtil {
public:
	template <unsigned int DIM, unsigned int WIDTH>
	static std::set<Entry<DIM, WIDTH>*> generateUniqueRandomEntries(size_t nUniqueEntries);

	template <unsigned int DIM, unsigned int WIDTH>
	static void writeAverageInsertTimeOfDimension(size_t runNumber, std::vector<Entry<DIM, WIDTH>*> entries);
	template <unsigned int DIM, unsigned int WIDTH>
	static void plotAverageInsertTimePerDimension(std::string file);
	static void plotAverageInsertTimePerDimensionRandom();

	template <unsigned int DIM, unsigned int WIDTH>
	static void plotAverageInsertTimePerNumberOfEntries(std::vector<std::vector<Entry<DIM, WIDTH>*>> entries);
	template <unsigned int DIM, unsigned int WIDTH>
	static void plotAverageInsertTimePerNumberOfEntries(std::string file);
	static void plotAverageInsertTimePerNumberOfEntriesRandom();

	static void plotTimeSeriesOfInserts();

private:
	static void plot(std::string gnuplotFileName);
	static void clearPlotFile(std::string dataFileName);
	static std::ofstream* openPlotFile(std::string dataFileName, bool removePreviousData);
	template <unsigned int DIM, unsigned int WIDTH>
	static inline std::vector<Entry<DIM, WIDTH>*> generateUniqueRandomEntriesList(size_t nUniqueEntries);
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
set<Entry<DIM, WIDTH>*> PlotUtil::generateUniqueRandomEntries(size_t nUniqueEntries) {
	srand(time(NULL));
	set<Entry<DIM, WIDTH>*> randomDimEntries;
	for (size_t nEntry = 0; nEntry < nUniqueEntries; nEntry++) {
		vector<unsigned long>* entryValues = new vector<unsigned long>(DIM);
		for (size_t d = 0; d < DIM; d++) {
			entryValues->at(d) = rand() % (1ul << WIDTH);
		}
		Entry<DIM, WIDTH>* entry = new Entry<DIM, WIDTH>(*entryValues, nEntry);
		bool inserted = randomDimEntries.insert(entry).second;
		delete entryValues;
		if (!inserted) {
			nEntry--;
			delete entry;
		}
	}

	return randomDimEntries;
}

template <unsigned int DIM, unsigned int WIDTH>
std::vector<Entry<DIM, WIDTH>*> PlotUtil::generateUniqueRandomEntriesList(size_t nUniqueEntries) {
	set<Entry<DIM, WIDTH>*> randomDimEntriesSet = generateUniqueRandomEntries<DIM, WIDTH>(N_RANDOM_ENTRIES_AVERAGE_INSERT);
	vector<Entry<DIM, WIDTH>*> randomDimEntries(randomDimEntriesSet.begin(),
			randomDimEntriesSet.end());
	return randomDimEntries;
}

void PlotUtil::plot(string gnuplotFileName) {
	string path = GNUPLOT_FILE_PATH + gnuplotFileName + GNUPLOT_FILE_EXTENSION;
	string gnuplotCommand = "gnuplot -p '" + path + "'";
	system(gnuplotCommand.c_str());
}

template <unsigned int DIM, unsigned int WIDTH>
void PlotUtil::writeAverageInsertTimeOfDimension(size_t runNumber, vector<Entry<DIM, WIDTH>*> entries)  {
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

		unsigned int startInsertTime = clock();
		for (size_t iEntry = 0; iEntry < entries.size(); iEntry++) {
			Entry<DIM, WIDTH>* entry = entries[iEntry];
			phtree->insert(entry);
		}
		unsigned int totalInsertTicks = clock() - startInsertTime;
		unsigned int startLookupTime = clock();
		for (size_t iEntry = 0; iEntry < entries.size(); iEntry++) {
			Entry<DIM, WIDTH>* entry = entries[iEntry];
			bool contained = phtree->lookup(entry).first;
			assert (contained);
		}
		unsigned int totalLookupTicks = clock() - startLookupTime;
		phtree->accept(visitor);
		phtree->accept(sizeVisitor);
		phtree->accept(prefixVisitor);
		cout << "d=" << DIM << endl << *visitor << *prefixVisitor << *sizeVisitor << endl;
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
			float insertMs = (float (insertTicks) / entries.size() / (CLOCKS_PER_SEC / 1000));
			float lookupMs = (float (lookupTicks) / entries.size() / (CLOCKS_PER_SEC / 1000));
			float totalSizeBit = (float(totalAhcBitSize + totalLhcBitSize + totalTreeBitSize)) / entries.size();
			(*plotFile) << runNumber
				<< "\t" << DIM
				<< "\t"	<< insertMs
				<< "\t" << lookupMs
				<< "\t"	<< nAHCNodes
				<< "\t" << nLHCNodes
				<< "\t" << (float(totalAhcBitSize) / entries.size() / DIM)
				<< "\t" << (float(totalLhcBitSize) / entries.size() / DIM)
				<< "\t" << (float(totalTreeBitSize) / entries.size() / DIM) << "\n";
			cout << runNumber << "\t" << DIM << "\t" << insertMs << "\t\t" << lookupMs  << "\t\t" << totalSizeBit << endl;

		// clear
		delete phtree;
		delete visitor;
		delete sizeVisitor;
		delete prefixVisitor;
		plotFile->close();
		delete plotFile;

		for (unsigned i = 0; i < entries.size(); ++i) delete entries.at(i);
}

template <unsigned int DIM, unsigned int WIDTH>
void PlotUtil::plotAverageInsertTimePerDimension(std::string file) {
	cout << "loading entries from file...";
	vector<Entry<DIM, WIDTH>*> entries = FileInputUtil::readEntries<DIM, WIDTH>(file);
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
		case 1: {
			vector<Entry<1, BIT_LENGTH>*> randomDimEntries =
					generateUniqueRandomEntriesList<1, BIT_LENGTH>(N_RANDOM_ENTRIES_AVERAGE_INSERT);
			writeAverageInsertTimeOfDimension<1, BIT_LENGTH>(test, randomDimEntries);
			break;
		}
		case 2: {
			vector<Entry<2, BIT_LENGTH>*> randomDimEntries =
								generateUniqueRandomEntriesList<2, BIT_LENGTH>(N_RANDOM_ENTRIES_AVERAGE_INSERT);
						writeAverageInsertTimeOfDimension<2, BIT_LENGTH>(test, randomDimEntries);
			break;
		}
		case 3: {
			vector<Entry<3, BIT_LENGTH>*> randomDimEntries =
								generateUniqueRandomEntriesList<3, BIT_LENGTH>(N_RANDOM_ENTRIES_AVERAGE_INSERT);
						writeAverageInsertTimeOfDimension<3, BIT_LENGTH>(test, randomDimEntries);
			break;
		}
		case 4: {
			vector<Entry<4, BIT_LENGTH>*> randomDimEntries =
								generateUniqueRandomEntriesList<4, BIT_LENGTH>(N_RANDOM_ENTRIES_AVERAGE_INSERT);
						writeAverageInsertTimeOfDimension<4, BIT_LENGTH>(test, randomDimEntries);
			break;
		}
		case 5: {
			vector<Entry<5, BIT_LENGTH>*> randomDimEntries =
								generateUniqueRandomEntriesList<5, BIT_LENGTH>(N_RANDOM_ENTRIES_AVERAGE_INSERT);
						writeAverageInsertTimeOfDimension<5, BIT_LENGTH>(test, randomDimEntries);
			break;
		}
		case 6: {
			vector<Entry<6, BIT_LENGTH>*> randomDimEntries =
								generateUniqueRandomEntriesList<6, BIT_LENGTH>(N_RANDOM_ENTRIES_AVERAGE_INSERT);
						writeAverageInsertTimeOfDimension<6, BIT_LENGTH>(test, randomDimEntries);
			break;
		}
		case 7: {
			vector<Entry<7, BIT_LENGTH>*> randomDimEntries =
								generateUniqueRandomEntriesList<7, BIT_LENGTH>(N_RANDOM_ENTRIES_AVERAGE_INSERT);
						writeAverageInsertTimeOfDimension<7, BIT_LENGTH>(test, randomDimEntries);
			break;
		}
		case 8: {
			vector<Entry<8, BIT_LENGTH>*> randomDimEntries =
								generateUniqueRandomEntriesList<8, BIT_LENGTH>(N_RANDOM_ENTRIES_AVERAGE_INSERT);
						writeAverageInsertTimeOfDimension<8, BIT_LENGTH>(test, randomDimEntries);
			break;
		}
		case 9: {
			vector<Entry<9, BIT_LENGTH>*> randomDimEntries =
								generateUniqueRandomEntriesList<9, BIT_LENGTH>(N_RANDOM_ENTRIES_AVERAGE_INSERT);
						writeAverageInsertTimeOfDimension<9, BIT_LENGTH>(test, randomDimEntries);
			break;
		}
		case 10: {
			vector<Entry<10, BIT_LENGTH>*> randomDimEntries =
								generateUniqueRandomEntriesList<10, BIT_LENGTH>(N_RANDOM_ENTRIES_AVERAGE_INSERT);
						writeAverageInsertTimeOfDimension<10, BIT_LENGTH>(test, randomDimEntries);
			break;
		}
		case 11: {
			vector<Entry<11, BIT_LENGTH>*> randomDimEntries =
								generateUniqueRandomEntriesList<11, BIT_LENGTH>(N_RANDOM_ENTRIES_AVERAGE_INSERT);
						writeAverageInsertTimeOfDimension<11, BIT_LENGTH>(test, randomDimEntries);
			break;
		}
		case 14: {
			vector<Entry<14, BIT_LENGTH>*> randomDimEntries =
								generateUniqueRandomEntriesList<14, BIT_LENGTH>(N_RANDOM_ENTRIES_AVERAGE_INSERT);
						writeAverageInsertTimeOfDimension<14, BIT_LENGTH>(test, randomDimEntries);
			break;
		}
		default:
			throw std::runtime_error(
					"currently the boiler plate code only supports dim <= 11, 14");
		}
	}

	// step 2: call Gnuplot
	cout << "calling gnuplot..." << endl;
	plot(AVERAGE_INSERT_DIM_PLOT_NAME);
}

template <unsigned int DIM, unsigned int WIDTH>
void PlotUtil::plotAverageInsertTimePerNumberOfEntries(vector<vector<Entry<DIM, WIDTH>*>> entries) {
		vector<unsigned int> insertTicks(entries.size());
		vector<unsigned int> lookupTicks(entries.size());
		vector<unsigned int> nAHCNodes(entries.size());
		vector<unsigned int> nLHCNodes(entries.size());
		vector<unsigned int> totalLhcBitSize(entries.size());
		vector<unsigned int> totalAhcBitSize(entries.size());
		vector<unsigned int> totalTreeBitSize(entries.size());

		cout << "start insertions...";
		CountNodeTypesVisitor<DIM>* visitor = new CountNodeTypesVisitor<DIM>();
		SizeVisitor<DIM>* sizeVisitor = new SizeVisitor<DIM>();
		PrefixSharingVisitor<DIM>* prefixVisitor = new PrefixSharingVisitor<DIM>();
		for (size_t test = 0; test < entries.size(); test++) {
			PHTree<DIM, WIDTH>* tree = new PHTree<DIM, WIDTH>();
			CALLGRIND_START_INSTRUMENTATION;
			unsigned int startInsertTime = clock();
			for (size_t iEntry = 0; iEntry < entries[test].size(); iEntry++) {
				Entry<DIM, WIDTH>* entry = entries[test][iEntry];
				tree->insert(entry);
			}
			unsigned int totalInsertTicks = clock() - startInsertTime;
			CALLGRIND_STOP_INSTRUMENTATION;
			unsigned int startLookupTime = clock();
			for (size_t iEntry = 0; iEntry < entries[test].size(); iEntry++) {
				Entry<DIM, WIDTH>* entry = entries[test][iEntry];
				tree->lookup(entry);
			}
			unsigned int totalLookupTicks = clock() - startLookupTime;
			tree->accept(visitor);
			tree->accept(sizeVisitor);
			tree->accept(prefixVisitor);
			cout << "n=" << entries[test].size() << endl << *visitor << *prefixVisitor << *sizeVisitor << endl;
			insertTicks.at(test) = totalInsertTicks;
			lookupTicks.at(test) = totalLookupTicks;
			nAHCNodes.at(test) = visitor->getNumberOfVisitedAHCNodes();
			nLHCNodes.at(test) = visitor->getNumberOfVisitedLHCNodes();
			totalLhcBitSize.at(test) = sizeVisitor->getTotalLhcBitSize();
			totalAhcBitSize.at(test) = sizeVisitor->getTotalAhcBitSize();
			totalTreeBitSize.at(test) = sizeVisitor->getTotalTreeBitSize();

			visitor->reset();
			sizeVisitor->reset();
			prefixVisitor->reset();
			delete tree;
			for (size_t iEntry = 0; iEntry < entries[test].size(); iEntry++) {
				Entry<DIM, WIDTH>* entry = entries[test][iEntry];
				delete entry;
			}
		}
		delete visitor;
		delete sizeVisitor;
		delete prefixVisitor;

		cout << " ok" << endl;
		// write gathered data into a file
		ofstream* plotFile = openPlotFile(AVERAGE_INSERT_ENTRIES_PLOT_NAME, true);
		for (size_t test = 0; test < entries.size(); test++) {
			(*plotFile) << test << "\t"
					<< entries[test].size() << "\t"
					<< (float (insertTicks[test]) / entries[test].size() / CLOCKS_PER_SEC * 1000) << "\t"
					<< (float (lookupTicks[test]) / entries[test].size() / CLOCKS_PER_SEC * 1000) << "\t"
					<< nAHCNodes.at(test) << "\t"
					<< nLHCNodes.at(test) << "\t"
					<< (float(totalAhcBitSize.at(test)) / entries[test].size() / ENTRY_DIM_INSERT_SERIES) << "\t"
					<< (float(totalLhcBitSize.at(test)) / entries[test].size() / ENTRY_DIM_INSERT_SERIES) << "\t"
					<< (float(totalTreeBitSize.at(test)) / entries[test].size() / ENTRY_DIM_INSERT_SERIES) << "\n";
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
	vector<vector<Entry<ENTRY_DIM_INSERT_SERIES, BIT_LENGTH>*>> testEntries(numberOfEntriesSize);
	vector<size_t> bitLengths(numberOfEntriesSize);

	for (unsigned test = 0; test < numberOfEntriesSize; ++test) {
		set<Entry<ENTRY_DIM_INSERT_SERIES, BIT_LENGTH>*> uniqueEntries =
				generateUniqueRandomEntries<ENTRY_DIM_INSERT_SERIES, BIT_LENGTH>(numberOfEntries[test]);
		vector<Entry<ENTRY_DIM_INSERT_SERIES, BIT_LENGTH>*> entries(uniqueEntries.begin(), uniqueEntries.end());
		testEntries.at(test) = entries;
	}

	plotAverageInsertTimePerNumberOfEntries<ENTRY_DIM_INSERT_SERIES, BIT_LENGTH>(testEntries);
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
	set<Entry<ENTRY_DIM, BIT_LENGTH>*> entries = generateUniqueRandomEntries<ENTRY_DIM, BIT_LENGTH>(N_RANDOM_ENTRIES_INSERT_SERIES);
	PHTree<ENTRY_DIM, BIT_LENGTH> phtree;
	ofstream* plotFile = openPlotFile(INSERT_SERIES_PLOT_NAME, true);

	CountNodeTypesVisitor<ENTRY_DIM>* visitor = new CountNodeTypesVisitor<ENTRY_DIM>();
	AssertionVisitor<ENTRY_DIM>* assertVisitor = new AssertionVisitor<ENTRY_DIM>();
	SizeVisitor<ENTRY_DIM>* sizeVisitor = new SizeVisitor<ENTRY_DIM>();
	uint64_t startInsert;
	uint64_t startLookup;
	try {
		size_t iEntry = 0;
		for (auto entry : entries) {
			cout << "inserting: " << *entry << endl;
			assert (!phtree.lookup(entry).first && "should not contain the entry before insertion");
			startInsert = RDTSC();
			phtree.insert(entry);
			uint64_t totalInsertTicks = RDTSC() - startInsert;
			cout << phtree << endl;
			phtree.accept(assertVisitor);
			phtree.accept(visitor);
			phtree.accept(sizeVisitor);
			startLookup = RDTSC();
			pair<bool, int> contained = phtree.lookup(entry);
			uint64_t totalLookupTicks = RDTSC() - startLookup;
			assert (contained.first && contained.second == entry->id_ && "should contain the entry after insertion");
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

	plotFile->close();
	delete plotFile;
	delete visitor;
	delete assertVisitor;
	delete sizeVisitor;

	plot(INSERT_SERIES_PLOT_NAME);
}

#endif /* PLOTUTIL_H_ */
