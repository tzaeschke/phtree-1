/*
 * PlotUtil.cpp
 *
 *  Created on: Mar 3, 2016
 *      Author: max
 */

#include "PlotUtil.h"

#include <fstream>
#include <stdlib.h>
#include <time.h>
#include <ctime>
#include <algorithm>
#include <assert.h>

#include "Entry.h"
#include "PHTree.h"
#include "visitors/CountNodeTypesVisitor.h"
#include "visitors/AssertionVisitor.h"
#include "visitors/SizeVisitor.h"
#include "util/FileInputUtil.h"
#include "util/rdtsc.h"

using namespace std;

set<Entry*> PlotUtil::generateUniqueRandomEntries(size_t dim, size_t bitLength, size_t nUniqueEntries) {
	srand(time(NULL));
	set<Entry*> randomDimEntries;
	for (size_t nEntry = 0; nEntry < nUniqueEntries; nEntry++) {
		vector<unsigned long>* entryValues = new vector<unsigned long>(dim);
		for (size_t d = 0; d < dim; d++) {
			entryValues->at(d) = rand() % (1ul << bitLength);
		}
		Entry* entry = new Entry(*entryValues, bitLength, nEntry);
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

void PlotUtil::plotAverageInsertTimePerDimension(vector<vector<Entry*>> entries, vector<size_t> dimensions, vector<size_t> bitLengths)  {
		cout << "inserting all entries into a PH-Tree while logging the time per insertion..." << endl;

		vector<PHTree*> phtrees;
		for (size_t test = 0; test < dimensions.size(); test++) {
			PHTree* tree = new PHTree(dimensions[test], bitLengths[test]);
			phtrees.push_back(tree);
		}

		// clock() -> insert all entries of one dim into the appropriate tree -> clock()
		vector<unsigned int> insertTicks(dimensions.size());
		vector<unsigned int> lookupTicks(dimensions.size());
		vector<unsigned int> nAHCNodes(dimensions.size());
		vector<unsigned int> nLHCNodes(dimensions.size());
		vector<unsigned int> totalLhcByteSize(dimensions.size());
		vector<unsigned int> totalAhcByteSize(dimensions.size());
		CountNodeTypesVisitor* visitor = new CountNodeTypesVisitor();
		SizeVisitor* sizeVisitor = new SizeVisitor();
		for (size_t test = 0; test < dimensions.size(); test++) {
			unsigned int startInsertTime = clock();
			for (size_t iEntry = 0; iEntry < entries[test].size(); iEntry++) {
				Entry* entry = entries[test][iEntry];
				phtrees[test]->insert(entry);
			}
			unsigned int totalInsertTicks = clock() - startInsertTime;
			unsigned int startLookupTime = clock();
			for (size_t iEntry = 0; iEntry < entries[test].size(); iEntry++) {
				Entry* entry = entries[test][iEntry];
				bool contained = phtrees[test]->lookup(entry).first;
				assert (contained);
			}
			unsigned int totalLookupTicks = clock() - startLookupTime;
			phtrees[test]->accept(visitor);
			phtrees[test]->accept(sizeVisitor);
			insertTicks.at(test) = totalInsertTicks;
			lookupTicks.at(test) = totalLookupTicks;
			nAHCNodes.at(test) = visitor->getNumberOfVisitedAHCNodes();
			nLHCNodes.at(test) = visitor->getNumberOfVisitedLHCNodes();
			totalAhcByteSize.at(test) = sizeVisitor->getTotalAhcByteSize();
			totalLhcByteSize.at(test) = sizeVisitor->getTotalLhcByteSize();
			visitor->reset();
			sizeVisitor->reset();
		}

		for (size_t test = 0; test < dimensions.size(); test++) {
			delete phtrees[test];
		}

		// write gathered data into a file
		ofstream* plotFile = openPlotFile(AVERAGE_INSERT_DIM_PLOT_NAME);
		cout << "\tdim\tinsert [ms]\t\tlookup [ms]\t\tsize [Mbyte]" << endl;
		for (size_t test = 0; test < dimensions.size(); test++) {
			float insertMs = (float (insertTicks[test]) / entries[test].size() / (CLOCKS_PER_SEC / 1000));
			float lookupMs = (float (lookupTicks[test]) / entries[test].size() / (CLOCKS_PER_SEC / 1000));
			(*plotFile) << test
				<< "\t" << dimensions[test]
				<< "\t"	<< insertMs
				<< "\t" << lookupMs
				<< "\t"	<< nAHCNodes.at(test)
				<< "\t" << nLHCNodes.at(test)
				<< "\t" << (float(totalAhcByteSize.at(test)) / 1000000)
				<< "\t" << (float(totalLhcByteSize.at(test)) / 1000000) << "\n";
			float totalSizeMByte = float(totalAhcByteSize.at(test) + totalLhcByteSize.at(test)) / 1000000;
			cout << test << "\t" << dimensions[test] << "\t" << insertMs << "\t\t" << lookupMs  << "\t\t" << totalSizeMByte << endl;
		}

		// clear
		delete visitor;
		delete sizeVisitor;
		insertTicks.clear();
		lookupTicks.clear();
		nAHCNodes.clear();
		nLHCNodes.clear();
		totalAhcByteSize.clear();
		totalLhcByteSize.clear();
		plotFile->close();
		delete plotFile;

		// step 2: call Gnuplot
		cout << "calling gnuplot..." << endl;
		plot(AVERAGE_INSERT_DIM_PLOT_NAME);
}

void PlotUtil::plotAverageInsertTimePerDimension(std::string file, size_t bitLength) {
	cout << "loading entries from file...";
	vector<Entry*> entries = FileInputUtil::readEntries(file, bitLength);
	vector<vector<Entry*>> singleColumnEntries(1);
	singleColumnEntries.at(0) = entries;
	vector<size_t> dimensions(1);
	dimensions.at(0) = entries.at(0)->getDimensions();
	vector<size_t> bitLengths(1);
	bitLengths.at(0) = bitLength;
	assert (entries.at(0)->getBitLength() == bitLength);
	cout << " ok" << endl;

	plotAverageInsertTimePerDimension(singleColumnEntries, dimensions, bitLengths);
}

void PlotUtil::plotAverageInsertTimePerDimensionRandom() {
	size_t dimTests[] = INSERT_ENTRY_DIMS;
	size_t dimTestsSize = sizeof(dimTests) / sizeof(*dimTests);
	vector<vector<Entry*>> randomEntries;
	vector<size_t> dimensions;
	vector<size_t> bitLengths;
	for (size_t test = 0; test < dimTestsSize; test++) {
		set<Entry*> randomDimEntriesSet = generateUniqueRandomEntries(dimTests[test], BIT_LENGTH, N_RANDOM_ENTRIES_AVERAGE_INSERT);
		vector<Entry*> randomDimEntries(randomDimEntriesSet.begin(), randomDimEntriesSet.end());
		randomEntries.push_back(randomDimEntries);
		dimensions.push_back(dimTests[test]);
		bitLengths.push_back(BIT_LENGTH);
	}

	plotAverageInsertTimePerDimension(randomEntries, dimensions, bitLengths);
}

void PlotUtil::plotAverageInsertTimePerNumberOfEntries(vector<vector<Entry*>> entries, vector<size_t> bitLengths) {
		vector<unsigned int> insertTicks(entries.size());
		vector<unsigned int> lookupTicks(entries.size());
		vector<unsigned int> nAHCNodes(entries.size());
		vector<unsigned int> nLHCNodes(entries.size());
		vector<unsigned int> totalLhcByteSize(entries.size());
		vector<unsigned int> totalAhcByteSize(entries.size());

		cout << "start insertions...";
		CountNodeTypesVisitor* visitor = new CountNodeTypesVisitor();
		SizeVisitor* sizeVisitor = new SizeVisitor();
		for (size_t test = 0; test < entries.size(); test++) {
			PHTree* tree = new PHTree(ENTRY_DIM_INSERT_SERIES, bitLengths[test]);
			unsigned int startInsertTime = clock();
			for (size_t iEntry = 0; iEntry < entries[test].size(); iEntry++) {
				Entry* entry = entries[test][iEntry];
				tree->insert(entry);
			}
			unsigned int totalInsertTicks = clock() - startInsertTime;
			unsigned int startLookupTime = clock();
			for (size_t iEntry = 0; iEntry < entries[test].size(); iEntry++) {
				Entry* entry = entries[test][iEntry];
				tree->lookup(entry);
			}
			unsigned int totalLookupTicks = clock() - startLookupTime;
			tree->accept(visitor);
			tree->accept(sizeVisitor);
			insertTicks.at(test) = totalInsertTicks;
			lookupTicks.at(test) = totalLookupTicks;
			nAHCNodes.at(test) = visitor->getNumberOfVisitedAHCNodes();
			nLHCNodes.at(test) = visitor->getNumberOfVisitedLHCNodes();
			totalLhcByteSize.at(test) = sizeVisitor->getTotalLhcByteSize();
			totalAhcByteSize.at(test) = sizeVisitor->getTotalAhcByteSize();

			visitor->reset();
			sizeVisitor->reset();
			delete tree;
			for (size_t iEntry = 0; iEntry < entries[test].size(); iEntry++) {
				Entry* entry = entries[test][iEntry];
				delete entry;
			}
		}
		delete visitor;

		cout << " ok" << endl;
		// write gathered data into a file
		ofstream* plotFile = openPlotFile(AVERAGE_INSERT_ENTRIES_PLOT_NAME);
		for (size_t test = 0; test < entries.size(); test++) {
			(*plotFile) << test << "\t"
					<< entries[test].size() << "\t"
					<< (float (insertTicks[test]) / entries[test].size() / CLOCKS_PER_SEC * 1000) << "\t"
					<< (float (lookupTicks[test]) / entries[test].size() / CLOCKS_PER_SEC * 1000) << "\t"
					<< nAHCNodes.at(test) << "\t"
					<< nLHCNodes.at(test) << "\t"
					<< (float(totalAhcByteSize.at(test)) / 1000000) << "\t"
					<< (float(totalLhcByteSize.at(test)) / 1000000) << "\n";
		}
		plotFile->close();
		delete plotFile;

		// step 2: call Gnuplot
		cout << "calling gnuplot...";
		plot(AVERAGE_INSERT_ENTRIES_PLOT_NAME);
		cout << " ok" << endl;
}

void PlotUtil::plotAverageInsertTimePerNumberOfEntries(std::string file, size_t bitLength) {
	vector<Entry*> entries = FileInputUtil::readEntries(file, bitLength);
	vector<vector<Entry*>> singleColumnEntries(1);
	singleColumnEntries.at(0) = entries;
	vector<size_t> bitLengths(1);
	bitLengths.at(0) = bitLength;

	plotAverageInsertTimePerNumberOfEntries(singleColumnEntries, bitLengths);
}

void PlotUtil::plotAverageInsertTimePerNumberOfEntriesRandom() {
	size_t numberOfEntries[] = INSERT_ENTRY_NUMBERS;
	size_t numberOfEntriesSize = sizeof(numberOfEntries) / sizeof(*numberOfEntries);
	vector<vector<Entry*>> testEntries(numberOfEntriesSize);
	vector<size_t> bitLengths(numberOfEntriesSize);

	for (unsigned test = 0; test < numberOfEntriesSize; ++test) {
		set<Entry*> uniqueEntries = generateUniqueRandomEntries(ENTRY_DIM_INSERT_SERIES, BIT_LENGTH, numberOfEntries[test]);
		vector<Entry*> entries(uniqueEntries.begin(), uniqueEntries.end());
		testEntries.at(test) = entries;
		bitLengths.at(test) = BIT_LENGTH;
	}

	plotAverageInsertTimePerNumberOfEntries(testEntries, bitLengths);
}

ofstream* PlotUtil::openPlotFile(std::string dataFileName) {
	std::string path = PLOT_DATA_PATH + dataFileName + PLOT_DATA_EXTENSION;
	ofstream* plotFile = new ofstream();
	plotFile->open(path.c_str(), ofstream::out | ofstream::trunc);
	return plotFile;
}

void PlotUtil::plotTimeSeriesOfInserts() {
	set<Entry*> entries = generateUniqueRandomEntries(ENTRY_DIM, BIT_LENGTH, N_RANDOM_ENTRIES_INSERT_SERIES);
	PHTree phtree(ENTRY_DIM, BIT_LENGTH);
	ofstream* plotFile = openPlotFile(INSERT_SERIES_PLOT_NAME);

	CountNodeTypesVisitor* visitor = new CountNodeTypesVisitor();
	AssertionVisitor* assertVisitor = new AssertionVisitor();
	SizeVisitor* sizeVisitor = new SizeVisitor();
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

	plot(INSERT_SERIES_PLOT_NAME);
}
