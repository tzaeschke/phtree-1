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

#define INSERT_ENTRY_DIMS {3, 5, 8, 11, 14};
#define INSERT_ENTRY_NUMBERS {100000, 1000000};

#define N_REPETITIONS 10
#define N_RANDOM_ENTRIES_AVERAGE_INSERT 100000
#define N_RANDOM_ENTRIES_INSERT_SERIES 100

template <unsigned int DIM>
class Entry;

class PlotUtil {
public:
	template <unsigned int DIM>
	static std::set<Entry<DIM>*> generateUniqueRandomEntries(size_t dim, size_t bitLength, size_t nUniqueEntries);

	template <unsigned int DIM>
	static void writeAverageInsertTimeOfDimension(size_t runNumber, std::vector<Entry<DIM>*> entries, size_t bitLength);
	template <unsigned int DIM>
	static void plotAverageInsertTimePerDimension(std::string file, size_t bitLength);
	static void plotAverageInsertTimePerDimensionRandom();

	template <unsigned int DIM>
	static void plotAverageInsertTimePerNumberOfEntries(std::vector<std::vector<Entry<DIM>*>> entries, std::vector<size_t> bitLengths);
	template <unsigned int DIM>
	static void plotAverageInsertTimePerNumberOfEntries(std::string file, size_t bitLength);
	static void plotAverageInsertTimePerNumberOfEntriesRandom();

	template <unsigned int DIM>
	static void plotTimeSeriesOfInserts();

private:
	static void plot(std::string gnuplotFileName);
	static void clearPlotFile(std::string dataFileName);
	static std::ofstream* openPlotFile(std::string dataFileName, bool removePreviousData);
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

template <unsigned int DIM>
set<Entry<DIM>*> PlotUtil::generateUniqueRandomEntries(size_t dim, size_t bitLength, size_t nUniqueEntries) {
	srand(time(NULL));
	set<Entry<DIM>*> randomDimEntries;
	for (size_t nEntry = 0; nEntry < nUniqueEntries; nEntry++) {
		vector<unsigned long>* entryValues = new vector<unsigned long>(dim);
		for (size_t d = 0; d < dim; d++) {
			entryValues->at(d) = rand() % (1ul << bitLength);
		}
		Entry<DIM>* entry = new Entry<DIM>(*entryValues, bitLength, nEntry);
		bool inserted = randomDimEntries.insert(entry).second;
		delete entryValues;
		if (!inserted) {
			nEntry--;
		}
	}

	return randomDimEntries;
}

void PlotUtil::plot(string gnuplotFileName) {
	string path = GNUPLOT_FILE_PATH + gnuplotFileName + GNUPLOT_FILE_EXTENSION;
	string gnuplotCommand = "gnuplot -p '" + path + "'";
	system(gnuplotCommand.c_str());
}

template <unsigned int DIM>
void PlotUtil::writeAverageInsertTimeOfDimension(size_t runNumber, vector<Entry<DIM>*> entries, size_t bitLength)  {
		cout << "inserting all entries into a PH-Tree while logging the time per insertion..." << endl;

		PHTree<DIM>* phtree = new PHTree<DIM>(bitLength);

		// clock() -> insert all entries of one dim into the appropriate tree -> clock()
		unsigned int insertTicks = 0;
		unsigned int lookupTicks = 0;
		unsigned int nAHCNodes = 0;
		unsigned int nLHCNodes = 0;
		unsigned int totalLhcBitSize = 0;
		unsigned int totalAhcBitSize = 0;
		CountNodeTypesVisitor<DIM>* visitor = new CountNodeTypesVisitor<DIM>();
		SizeVisitor<DIM>* sizeVisitor = new SizeVisitor<DIM>();
		PrefixSharingVisitor<DIM>* prefixVisitor = new PrefixSharingVisitor<DIM>();

		unsigned int startInsertTime = clock();
		for (size_t iEntry = 0; iEntry < entries.size(); iEntry++) {
			Entry<DIM>* entry = entries[iEntry];
			phtree->insert(entry);
		}
		unsigned int totalInsertTicks = clock() - startInsertTime;
		unsigned int startLookupTime = clock();
		for (size_t iEntry = 0; iEntry < entries.size(); iEntry++) {
			Entry<DIM>* entry = entries[iEntry];
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

		// write gathered data into a file
		ofstream* plotFile = openPlotFile(AVERAGE_INSERT_DIM_PLOT_NAME, false);
		cout << "\tdim\tinsert [ms]\t\tlookup [ms]\t\tsize [bit per dimension]" << endl;
			float insertMs = (float (insertTicks) / entries.size() / (CLOCKS_PER_SEC / 1000));
			float lookupMs = (float (lookupTicks) / entries.size() / (CLOCKS_PER_SEC / 1000));
			float totalSizeBit = (float(totalAhcBitSize + totalLhcBitSize)) / entries.size();
			(*plotFile) << runNumber
				<< "\t" << DIM
				<< "\t"	<< insertMs
				<< "\t" << lookupMs
				<< "\t"	<< nAHCNodes
				<< "\t" << nLHCNodes
				<< "\t" << (float(totalAhcBitSize) / entries.size() / DIM)
				<< "\t" << (float(totalLhcBitSize) / entries.size() / DIM) << "\n";
			cout << runNumber << "\t" << DIM << "\t" << insertMs << "\t\t" << lookupMs  << "\t\t" << totalSizeBit << endl;

		// clear
		delete phtree;
		delete visitor;
		delete sizeVisitor;
		delete prefixVisitor;
		plotFile->close();
		delete plotFile;
}

template <unsigned int DIM>
void PlotUtil::plotAverageInsertTimePerDimension(std::string file, size_t bitLength) {
	cout << "loading entries from file...";
	vector<Entry<DIM>*> entries = FileInputUtil::readEntries<DIM>(file, bitLength);
	assert (entries.at(0)->getBitLength() == bitLength);
	cout << " ok" << endl;

	writeAverageInsertTimeOfDimension<DIM>(0, entries, bitLength);
}

void PlotUtil::plotAverageInsertTimePerDimensionRandom() {
	size_t dimTests[] = INSERT_ENTRY_DIMS
	;
	size_t dimTestsSize = sizeof(dimTests) / sizeof(*dimTests);
	clearPlotFile(AVERAGE_INSERT_DIM_PLOT_NAME);

	for (size_t test = 0; test < dimTestsSize; test++) {
		// resolve dynamic dimensions
		switch (dimTests[test]) {
		case 1: {
			set<Entry<1>*> randomDimEntriesSet1 =
					generateUniqueRandomEntries<1>(1, BIT_LENGTH,
							N_RANDOM_ENTRIES_AVERAGE_INSERT);
			vector<Entry<1>*> randomDimEntries1(randomDimEntriesSet1.begin(),
					randomDimEntriesSet1.end());
			writeAverageInsertTimeOfDimension<1>(test, randomDimEntries1,
			BIT_LENGTH);
			break;
		}
		case 2: {
			set<Entry<2>*> randomDimEntriesSet2 =
					generateUniqueRandomEntries<2>(2, BIT_LENGTH,
							N_RANDOM_ENTRIES_AVERAGE_INSERT);
			vector<Entry<2>*> randomDimEntries2(randomDimEntriesSet2.begin(),
					randomDimEntriesSet2.end());
			writeAverageInsertTimeOfDimension<2>(test, randomDimEntries2,
			BIT_LENGTH);
			break;
		}
		case 3: {
			set<Entry<3>*> randomDimEntriesSet3 =
					generateUniqueRandomEntries<3>(3, BIT_LENGTH,
							N_RANDOM_ENTRIES_AVERAGE_INSERT);
			vector<Entry<3>*> randomDimEntries3(randomDimEntriesSet3.begin(),
					randomDimEntriesSet3.end());
			writeAverageInsertTimeOfDimension<3>(test, randomDimEntries3,
			BIT_LENGTH);
			break;
		}
		case 4: {
			set<Entry<4>*> randomDimEntriesSet4 =
					generateUniqueRandomEntries<4>(4, BIT_LENGTH,
							N_RANDOM_ENTRIES_AVERAGE_INSERT);
			vector<Entry<4>*> randomDimEntries4(randomDimEntriesSet4.begin(),
					randomDimEntriesSet4.end());
			writeAverageInsertTimeOfDimension<4>(test, randomDimEntries4,
			BIT_LENGTH);
			break;
		}
		case 5: {
			set<Entry<5>*> randomDimEntriesSet5 =
					generateUniqueRandomEntries<5>(5, BIT_LENGTH,
							N_RANDOM_ENTRIES_AVERAGE_INSERT);
			vector<Entry<5>*> randomDimEntries5(randomDimEntriesSet5.begin(),
					randomDimEntriesSet5.end());
			writeAverageInsertTimeOfDimension<5>(test, randomDimEntries5,
			BIT_LENGTH);
			break;
		}
		case 6: {
			set<Entry<6>*> randomDimEntriesSet6 =
					generateUniqueRandomEntries<6>(6, BIT_LENGTH,
							N_RANDOM_ENTRIES_AVERAGE_INSERT);
			vector<Entry<6>*> randomDimEntries6(randomDimEntriesSet6.begin(),
					randomDimEntriesSet6.end());
			writeAverageInsertTimeOfDimension<6>(test, randomDimEntries6,
			BIT_LENGTH);
			break;
		}
		case 7: {
			set<Entry<7>*> randomDimEntriesSet7 =
					generateUniqueRandomEntries<7>(7, BIT_LENGTH,
							N_RANDOM_ENTRIES_AVERAGE_INSERT);
			vector<Entry<7>*> randomDimEntries7(randomDimEntriesSet7.begin(),
					randomDimEntriesSet7.end());
			writeAverageInsertTimeOfDimension<7>(test, randomDimEntries7,
			BIT_LENGTH);
			break;
		}
		case 8: {
			set<Entry<8>*> randomDimEntriesSet8 =
					generateUniqueRandomEntries<8>(8, BIT_LENGTH,
							N_RANDOM_ENTRIES_AVERAGE_INSERT);
			vector<Entry<8>*> randomDimEntries8(randomDimEntriesSet8.begin(),
					randomDimEntriesSet8.end());
			writeAverageInsertTimeOfDimension<8>(test, randomDimEntries8,
			BIT_LENGTH);
			break;
		}
		case 9: {
			set<Entry<9>*> randomDimEntriesSet9 =
					generateUniqueRandomEntries<9>(9, BIT_LENGTH,
							N_RANDOM_ENTRIES_AVERAGE_INSERT);
			vector<Entry<9>*> randomDimEntries9(randomDimEntriesSet9.begin(),
					randomDimEntriesSet9.end());
			writeAverageInsertTimeOfDimension<9>(test, randomDimEntries9,
			BIT_LENGTH);
			break;
		}
		case 10: {
			set<Entry<10>*> randomDimEntriesSet10 = generateUniqueRandomEntries<
					10>(10, BIT_LENGTH, N_RANDOM_ENTRIES_AVERAGE_INSERT);
			vector<Entry<10>*> randomDimEntries10(randomDimEntriesSet10.begin(),
					randomDimEntriesSet10.end());
			writeAverageInsertTimeOfDimension<10>(test, randomDimEntries10,
			BIT_LENGTH);
			break;
		}
		case 11: {
			set<Entry<11>*> randomDimEntriesSet11 = generateUniqueRandomEntries<
					11>(11, BIT_LENGTH, N_RANDOM_ENTRIES_AVERAGE_INSERT);
			vector<Entry<11>*> randomDimEntries11(randomDimEntriesSet11.begin(),
					randomDimEntriesSet11.end());
			writeAverageInsertTimeOfDimension<11>(test, randomDimEntries11,
			BIT_LENGTH);
			break;
		}
		case 14: {
			set<Entry<14>*> randomDimEntriesSet14 = generateUniqueRandomEntries<
					14>(14, BIT_LENGTH, N_RANDOM_ENTRIES_AVERAGE_INSERT);
			vector<Entry<14>*> randomDimEntries14(randomDimEntriesSet14.begin(),
					randomDimEntriesSet14.end());
			writeAverageInsertTimeOfDimension<14>(test, randomDimEntries14,
			BIT_LENGTH);
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

template <unsigned int DIM>
void PlotUtil::plotAverageInsertTimePerNumberOfEntries(vector<vector<Entry<DIM>*>> entries, vector<size_t> bitLengths) {
		vector<unsigned int> insertTicks(entries.size());
		vector<unsigned int> lookupTicks(entries.size());
		vector<unsigned int> nAHCNodes(entries.size());
		vector<unsigned int> nLHCNodes(entries.size());
		vector<unsigned int> totalLhcBitSize(entries.size());
		vector<unsigned int> totalAhcBitSize(entries.size());

		cout << "start insertions...";
		CountNodeTypesVisitor<DIM>* visitor = new CountNodeTypesVisitor<DIM>();
		SizeVisitor<DIM>* sizeVisitor = new SizeVisitor<DIM>();
		PrefixSharingVisitor<DIM>* prefixVisitor = new PrefixSharingVisitor<DIM>();
		for (size_t test = 0; test < entries.size(); test++) {
			PHTree<DIM>* tree = new PHTree<DIM>(bitLengths[test]);
			unsigned int startInsertTime = clock();
			for (size_t iEntry = 0; iEntry < entries[test].size(); iEntry++) {
				Entry<DIM>* entry = entries[test][iEntry];
				tree->insert(entry);
			}
			CALLGRIND_START_INSTRUMENTATION;
			unsigned int totalInsertTicks = clock() - startInsertTime;
			unsigned int startLookupTime = clock();
			for (size_t iEntry = 0; iEntry < entries[test].size(); iEntry++) {
				Entry<DIM>* entry = entries[test][iEntry];
				tree->lookup(entry);
			}
			unsigned int totalLookupTicks = clock() - startLookupTime;
			CALLGRIND_STOP_INSTRUMENTATION;
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

			visitor->reset();
			sizeVisitor->reset();
			prefixVisitor->reset();
			delete tree;
			for (size_t iEntry = 0; iEntry < entries[test].size(); iEntry++) {
				Entry<DIM>* entry = entries[test][iEntry];
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
					<< (float(totalLhcBitSize.at(test)) / entries[test].size() / ENTRY_DIM_INSERT_SERIES) << "\n";
		}
		plotFile->close();
		delete plotFile;

		// step 2: call Gnuplot
		cout << "calling gnuplot...";
		plot(AVERAGE_INSERT_ENTRIES_PLOT_NAME);
		cout << " ok" << endl;
}

template <unsigned int DIM>
void PlotUtil::plotAverageInsertTimePerNumberOfEntries(std::string file, size_t bitLength) {
	vector<Entry<DIM>*> entries = FileInputUtil::readEntries<DIM>(file, bitLength);
	vector<vector<Entry<DIM>*>> singleColumnEntries(1);
	singleColumnEntries.at(0) = entries;
	vector<size_t> bitLengths(1);
	bitLengths.at(0) = bitLength;

	plotAverageInsertTimePerNumberOfEntries<DIM>(singleColumnEntries, bitLengths);
}

void PlotUtil::plotAverageInsertTimePerNumberOfEntriesRandom() {
	size_t numberOfEntries[] = INSERT_ENTRY_NUMBERS;
	size_t numberOfEntriesSize = sizeof(numberOfEntries) / sizeof(*numberOfEntries);
	vector<vector<Entry<ENTRY_DIM_INSERT_SERIES>*>> testEntries(numberOfEntriesSize);
	vector<size_t> bitLengths(numberOfEntriesSize);

	for (unsigned test = 0; test < numberOfEntriesSize; ++test) {
		set<Entry<ENTRY_DIM_INSERT_SERIES>*> uniqueEntries = generateUniqueRandomEntries<ENTRY_DIM_INSERT_SERIES>(ENTRY_DIM_INSERT_SERIES, BIT_LENGTH, numberOfEntries[test]);
		vector<Entry<ENTRY_DIM_INSERT_SERIES>*> entries(uniqueEntries.begin(), uniqueEntries.end());
		testEntries.at(test) = entries;
		bitLengths.at(test) = BIT_LENGTH;
	}

	plotAverageInsertTimePerNumberOfEntries<ENTRY_DIM_INSERT_SERIES>(testEntries, bitLengths);
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

template <unsigned int DIM>
void PlotUtil::plotTimeSeriesOfInserts() {
	set<Entry<DIM>*> entries = generateUniqueRandomEntries<DIM>(ENTRY_DIM, BIT_LENGTH, N_RANDOM_ENTRIES_INSERT_SERIES);
	PHTree<DIM> phtree(ENTRY_DIM, BIT_LENGTH);
	ofstream* plotFile = openPlotFile(INSERT_SERIES_PLOT_NAME, true);

	CountNodeTypesVisitor<DIM>* visitor = new CountNodeTypesVisitor<DIM>();
	AssertionVisitor<DIM>* assertVisitor = new AssertionVisitor<DIM>();
	SizeVisitor<DIM>* sizeVisitor = new SizeVisitor<DIM>();
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
