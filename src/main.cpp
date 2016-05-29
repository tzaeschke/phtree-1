#include <iostream>
#include <string>
#include <vector>
#include <assert.h>

#include "Entry.h"
#include "PHTree.h"
#include "util/PlotUtil.h"
#include "util/rdtsc.h"
#include "visitors/CountNodeTypesVisitor.h"
#include "iterators/RangeQueryIterator.h"

using namespace std;

int mainSimpleExample() {
	const unsigned int bitLength = 8;
	Entry<2, bitLength> e1({ 74, 21 }, 1);
	Entry<2, bitLength> e2({ 75, 28 }, 2);
	Entry<2, bitLength> e3({ 124, 7 }, 3);
	Entry<2, bitLength> e4({ 65, 19 }, 4);
	Entry<2, bitLength> e5({ 75, 21 }, 5);

	CountNodeTypesVisitor<2>* visitor = new CountNodeTypesVisitor<2>();
	uint64_t sta = RDTSC();
	PHTree<2, bitLength>* phtree = new PHTree<2, bitLength>();
	phtree->insert(e1);
	cout << "CPU cycles per insert: " << RDTSC() - sta << endl;
	cout << *phtree;
	phtree->accept(visitor);
	cout << *visitor << endl;

	sta = RDTSC();
	phtree->insert(e2);
	cout << "CPU cycles per insert: " << RDTSC() - sta << endl;
	assert (phtree->lookup(e1).second == 1);
	assert (!phtree->lookup(e3).first);
	cout << *phtree;
	visitor->reset();
	phtree->accept(visitor);
	cout << *visitor << endl;

	sta = RDTSC();
	phtree->insert(e3);
	cout << "CPU cycles per insert: " << RDTSC() - sta << endl;
	assert (phtree->lookup(e3).second == 3);
	cout << *phtree;
	visitor->reset();
	phtree->accept(visitor);
	cout << *visitor << endl;

	sta = RDTSC();
	phtree->insert(e4);
	cout << "CPU cycles per insert: " << RDTSC() - sta << endl;
	assert (phtree->lookup(e2).second == 2);
	assert (phtree->lookup(e4).second == 4);
	cout << *phtree;
	visitor->reset();
	phtree->accept(visitor);
	cout << *visitor << endl;

	sta = RDTSC();
	phtree->insert(e5);
	cout << "CPU cycles per insert: " << RDTSC() - sta << endl;
	assert (phtree->lookup(e5).second == 5);
	cout << *phtree;
	visitor->reset();
	phtree->accept(visitor);
	cout << *visitor << endl;

	cout << "The following entries are in the range (0,0) - (100,100):" << endl;
	RangeQueryIterator<2, bitLength>* it = phtree->rangeQuery(
			Entry<2, bitLength>({0,0}, 0),
			Entry<2, bitLength>({100,100}, 0));
	while (it->hasNext()) {
		Entry<2, bitLength> entryInRange = it->next();
		cout << entryInRange << endl;
	}

	cout << "The following entries are in the range (65,10) - (150,20):" << endl;
	it = phtree->rangeQuery(
			Entry<2, bitLength>({65,10}, 0),
			Entry<2, bitLength>({150,20}, 0));
	while (it->hasNext()) {
		Entry<2, bitLength> entryInRange = it->next();
		cout << entryInRange << endl;
	}

	cout << "The following entries are in the range (74,20) - (74,21):" << endl;
	it = phtree->rangeQuery(
			Entry<2, bitLength>({74,20}, 0),
			Entry<2, bitLength>({74,21}, 0));
	while (it->hasNext()) {
		Entry<2, bitLength> entryInRange = it->next();
		cout << entryInRange << endl;
	}

	delete visitor;
	delete phtree;

	return 0;
}

int main(int argc, char* argv[]) {

	string debug = "debug";
	string plot = "plot";
	string rand = "rand";
	string benchmark = "benchmark";

	#ifndef NDEBUG
		cout << "assertions enabled!" << endl;
	#endif

	#ifdef PRINT
		cout << "printing enabled!" << endl;
	#endif

	if (argc != 2 || debug.compare(argv[1]) == 0) {
		return mainSimpleExample();
	} else if (plot.compare(argv[1]) == 0) {
//		PlotUtil::plotTimeSeriesOfInserts();
		PlotUtil::plotAverageInsertTimePerDimensionRandom();
		PlotUtil::plotAverageInsertTimePerNumberOfEntriesRandom();
		PlotUtil::plotRangeQueryTimePerPercentFilledRandom();
		return 0;
	} else if (rand.compare(argv[1]) == 0) {
		vector<size_t> nEntries;
		nEntries.push_back(500000);
		PlotUtil::plotAverageInsertTimePerNumberOfEntriesRandom(nEntries);
	} else if (benchmark.compare(argv[1]) == 0) {
		cout << "run a benchmark extracted from the Java implementation with 1M 3D 32-bit entries" << endl;
		PlotUtil::plotAverageInsertTimePerDimension<3, 32>("./benchmark_Java-extract_1M_3D_32bit.dat");
	} else {
		cerr << "Missing command line argument!" << endl << "valid: 'debug', 'plot', 'rand', 'benchmark'";
		return 1;
	}
};
